#include "server/server.h"

#include <utility>

void Server::enqueue(Command command) {
  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (!running_) {
      return;
    }
    commands_.push_back(std::move(command));
  }

  command_ready_.notify_one();
}

bool Server::process_one() {
  Command command;

  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (commands_.empty()) {
      return false;
    }
    command = std::move(commands_.front());
    commands_.pop_front();
  }

  std::visit([this](const auto& value) { handle(value); }, command);
  return true;
}

void Server::run() {
  while (true) {
    Command command;

    {
      std::unique_lock<std::mutex> lock(command_mutex_);
      command_ready_.wait(lock, [this] {
        return !commands_.empty() || !running_;
      });

      if (!running_ && commands_.empty()) {
        return;
      }

      command = std::move(commands_.front());
      commands_.pop_front();
    }

    std::visit([this](const auto& value) { handle(value); }, command);
  }
}

void Server::stop() {
  {
    std::lock_guard<std::mutex> lock(command_mutex_);
    running_ = false;
  }
  command_ready_.notify_all();
}

bool Server::try_pop_event(ServerEvent& event) {
  std::lock_guard<std::mutex> lock(event_mutex_);
  if (events_.empty()) {
    return false;
  }

  event = std::move(events_.front());
  events_.pop_front();
  return true;
}

void Server::handle(const CreateGame& command) {
  if (command.game_id.empty()) {
    reject("", command.request_id, CommandErrorCode::BadRequest,
        "Game id cannot be empty");
    return;
  }

  if (command.rows <= 0 || command.cols <= 0 || command.mines < 0 ||
      command.mines >= command.rows * command.cols) {
    reject("", command.request_id, CommandErrorCode::BadRequest,
        "Invalid board dimensions or mine count");
    return;
  }

  if (games_.find(command.game_id) != games_.end()) {
    reject("", command.request_id, CommandErrorCode::InvalidAction,
        "Game already exists");
    return;
  }

  auto game = std::make_unique<RunningGame>(
      command.rows, command.cols, command.mines);
  RunningGame& created_game = *game;
  games_.emplace(command.game_id, std::move(game));
  publish_snapshot(
      EventAudience::Game, "", command.game_id, command.request_id, created_game);
}

void Server::handle(const RevealTile& command) {
  RunningGame* game = find_game(command.game_id);
  if (game == nullptr) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::GameNotFound, "Game does not exist");
    return;
  }

  if (!game->game.board().contains(command.row, command.col)) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::BadRequest, "Tile is outside the board");
    return;
  }

  if (!game->game.reveal(command.row, command.col)) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::InvalidAction, "Tile cannot be revealed");
    return;
  }

  game->revision += 1;
  publish_snapshot(EventAudience::Game, command.connection_id,
      command.game_id, command.request_id, *game);
}

void Server::handle(const ToggleFlag& command) {
  RunningGame* game = find_game(command.game_id);
  if (game == nullptr) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::GameNotFound, "Game does not exist");
    return;
  }

  if (!game->game.board().contains(command.row, command.col)) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::BadRequest, "Tile is outside the board");
    return;
  }

  if (!game->game.toggle_flag(command.row, command.col)) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::InvalidAction, "Flag cannot be changed");
    return;
  }

  game->revision += 1;
  publish_snapshot(EventAudience::Game, command.connection_id,
      command.game_id, command.request_id, *game);
}

void Server::handle(const RestartGame& command) {
  RunningGame* game = find_game(command.game_id);
  if (game == nullptr) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::GameNotFound, "Game does not exist");
    return;
  }

  game->game.restart();
  game->revision += 1;
  publish_snapshot(EventAudience::Game, command.connection_id,
      command.game_id, command.request_id, *game);
}

void Server::handle(const RequestSnapshot& command) {
  RunningGame* game = find_game(command.game_id);
  if (game == nullptr) {
    reject(command.connection_id, command.request_id,
        CommandErrorCode::GameNotFound, "Game does not exist");
    return;
  }

  publish_snapshot(EventAudience::Connection, command.connection_id,
      command.game_id, command.request_id, *game);
}

void Server::publish_snapshot(
    EventAudience audience,
    const std::string& connection_id,
    const std::string& game_id,
    const std::string& request_id,
    const RunningGame& game) {
  GameSnapshot snapshot{
      audience,
      connection_id,
      game_id,
      request_id,
      game.revision,
      game.game.status(),
      game.game.elapsed_seconds(),
      game.game.board().get_counter(),
      game.game.board().rows(),
      game.game.board().cols(),
      {}};

  const Board& board = game.game.board();
  snapshot.tiles.reserve(board.rows() * board.cols());
  for (int row = 0; row < board.rows(); row += 1) {
    for (int col = 0; col < board.cols(); col += 1) {
      const Tile& tile = board.tile_at(row, col);
      bool reveal_mine = tile.is_mine() &&
          game.game.status() != GameStatus::Playing;
      if (!tile.is_revealed() && !reveal_mine) {
        snapshot.tiles.push_back(HiddenTileView{
            row, col, tile.flag_placed()});
        continue;
      }

      snapshot.tiles.push_back(RevealedTileView{
          row,
          col,
          tile.is_mine(),
          tile.is_mine() ? 0 : tile.get_adjacent_mines()});
    }
  }

  std::lock_guard<std::mutex> lock(event_mutex_);
  events_.push_back(std::move(snapshot));
}

void Server::reject(
    const std::string& connection_id,
    const std::string& request_id,
    CommandErrorCode code,
    const std::string& message) {
  std::lock_guard<std::mutex> lock(event_mutex_);
  events_.push_back(CommandRejected{connection_id, request_id, code, message});
}

Server::RunningGame* Server::find_game(const std::string& game_id) {
  auto found = games_.find(game_id);
  return found == games_.end() ? nullptr : found->second.get();
}
