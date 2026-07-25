#include "runtime.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace minesweeper {

Result GameRuntime::execute(
    const std::string& game_id,
    const Command& command) {
  if (game_id.empty()) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::BadRequest, "Game id cannot be empty"}};
  }

  return std::visit(
      [this, &game_id](const auto& value) {
        return handle(game_id, value);
      },
      command);
}

bool GameRuntime::has_game(const std::string& game_id) const {
  return games_.find(game_id) != games_.end();
}

GameCheckpoint GameRuntime::checkpoint() const {
  return checkpoint_at(Clock::now());
}

void GameRuntime::restore(const GameCheckpoint& checkpoint) {
  std::unordered_map<std::string, RunningGame> restored;
  restored.reserve(checkpoint.games.size());
  const Clock::time_point now = Clock::now();

  for (const GameRecord& saved : checkpoint.games) {
    if (saved.game_id.empty()) {
      throw std::invalid_argument("Saved game id cannot be empty");
    }
    minesweeper::validate_game(saved.state);

    if (!restored.emplace(saved.game_id, RunningGame{
            saved.revision, saved.state, now}).second) {
      throw std::invalid_argument("Saved game ids must be unique");
    }
  }

  games_ = std::move(restored);
}

Result GameRuntime::handle(
    const std::string& game_id,
    const CreateGame& command) {
  if (has_game(game_id)) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::InvalidAction, "Game already exists"}};
  }

  RunningGame candidate;
  try {
    const auto mines = minesweeper::choose_mines(
        command.rows, command.cols, command.mines, random_generator_);
    candidate.state = minesweeper::create_game(
        command.rows, command.cols, mines);
  } catch (const std::exception& error) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::BadRequest, error.what()}};
  }

  const Clock::time_point now = Clock::now();
  candidate.updated_at = now;
  commit(game_id, std::move(candidate), now);
  return {EventAudience::Game, snapshot(*find_game(game_id))};
}

Result GameRuntime::handle(
    const std::string& game_id,
    const RevealTile& command) {
  RunningGame* game = find_game(game_id);
  if (game == nullptr) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::GameNotFound, "Game does not exist"}};
  }

  const Clock::time_point now = Clock::now();
  RunningGame candidate = materialized_game(*game, now);
  const minesweeper::MoveResult move =
      minesweeper::reveal(candidate.state, command.position);
  if (move == minesweeper::MoveResult::OutOfBounds) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::BadRequest,
            "Tile is outside the board"}};
  }
  if (move != minesweeper::MoveResult::Applied) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::InvalidAction,
            "Tile cannot be revealed"}};
  }

  candidate.revision += 1;
  candidate.updated_at = now;
  commit(game_id, std::move(candidate), now);
  return {EventAudience::Game, snapshot(*find_game(game_id))};
}

Result GameRuntime::handle(
    const std::string& game_id,
    const ToggleFlag& command) {
  RunningGame* game = find_game(game_id);
  if (game == nullptr) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::GameNotFound, "Game does not exist"}};
  }

  const Clock::time_point now = Clock::now();
  RunningGame candidate = materialized_game(*game, now);
  const minesweeper::MoveResult move =
      minesweeper::toggle_flag(candidate.state, command.position);
  if (move == minesweeper::MoveResult::OutOfBounds) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::BadRequest,
            "Tile is outside the board"}};
  }
  if (move != minesweeper::MoveResult::Applied) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::InvalidAction,
            "Flag cannot be changed"}};
  }

  candidate.revision += 1;
  candidate.updated_at = now;
  commit(game_id, std::move(candidate), now);
  return {EventAudience::Game, snapshot(*find_game(game_id))};
}

Result GameRuntime::handle(
    const std::string& game_id,
    const RestartGame&) {
  RunningGame* game = find_game(game_id);
  if (game == nullptr) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::GameNotFound, "Game does not exist"}};
  }

  const Clock::time_point now = Clock::now();
  RunningGame candidate = materialized_game(*game, now);
  const int mine_count = static_cast<int>(std::count_if(
      candidate.state.board.tiles.begin(),
      candidate.state.board.tiles.end(),
      [](const minesweeper::Tile& tile) { return tile.mine; }));
  const auto mine_indexes = minesweeper::choose_mines(
      candidate.state.board.rows,
      candidate.state.board.cols,
      mine_count,
      random_generator_);
  auto replacement = minesweeper::create_game(
      candidate.state.board.rows,
      candidate.state.board.cols,
      mine_indexes);
  minesweeper::restart(candidate.state, std::move(replacement.board));

  candidate.revision += 1;
  candidate.updated_at = now;
  commit(game_id, std::move(candidate), now);
  return {EventAudience::Game, snapshot(*find_game(game_id))};
}

Result GameRuntime::handle(
    const std::string& game_id,
    const RequestSnapshot&) {
  const RunningGame* game = find_game(game_id);
  if (game == nullptr) {
    return {EventAudience::Connection,
        CommandError{CommandErrorCode::GameNotFound, "Game does not exist"}};
  }
  return {EventAudience::Connection, snapshot(*game)};
}

GameRuntime::RunningGame* GameRuntime::find_game(const std::string& game_id) {
  const auto found = games_.find(game_id);
  return found == games_.end() ? nullptr : &found->second;
}

GameRuntime::RunningGame GameRuntime::materialized_game(
    const RunningGame& game,
    Clock::time_point now) const {
  RunningGame materialized = game;
  if (materialized.state.status == minesweeper::GameStatus::Playing &&
      !materialized.state.paused) {
    materialized.state.elapsed_milliseconds +=
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - game.updated_at).count();
  }
  materialized.updated_at = now;
  return materialized;
}

GameSnapshot GameRuntime::snapshot(const RunningGame& game) const {
  const RunningGame visible = materialized_game(game, Clock::now());
  GameSnapshot result{
      visible.revision,
      visible.state.status,
      static_cast<int>(visible.state.elapsed_milliseconds / 1000),
      minesweeper::remaining_mines(visible.state.board),
      visible.state.board.rows,
      visible.state.board.cols,
      {}};

  const minesweeper::Board& board = visible.state.board;
  result.tiles.reserve(board.tiles.size());
  for (int row = 0; row < board.rows; row += 1) {
    for (int col = 0; col < board.cols; col += 1) {
      const minesweeper::Position position{row, col};
      const minesweeper::Tile& tile = minesweeper::tile_at(board, position);
      const bool reveal_mine = tile.mine &&
          visible.state.status != minesweeper::GameStatus::Playing;
      if (!tile.revealed && !reveal_mine) {
        result.tiles.push_back(HiddenTileView{row, col, tile.flagged});
        continue;
      }

      result.tiles.push_back(RevealedTileView{
          row,
          col,
          tile.mine,
          tile.mine ? 0 : minesweeper::adjacent_mines(board, position),
      });
    }
  }
  return result;
}

GameCheckpoint GameRuntime::checkpoint_at(Clock::time_point now) const {
  GameCheckpoint checkpoint;
  checkpoint.games.reserve(games_.size());
  for (const auto& [game_id, game] : games_) {
    const RunningGame materialized = materialized_game(game, now);
    checkpoint.games.push_back(
        GameRecord{game_id, materialized.revision, materialized.state});
  }
  std::sort(checkpoint.games.begin(), checkpoint.games.end(),
      [](const GameRecord& left, const GameRecord& right) {
        return left.game_id < right.game_id;
      });
  return checkpoint;
}

GameCheckpoint GameRuntime::checkpoint_with(
    const std::string& game_id,
    const RunningGame& candidate,
    Clock::time_point now) const {
  GameCheckpoint checkpoint;
  checkpoint.games.reserve(games_.size() + 1);
  bool replaced = false;
  for (const auto& [existing_id, game] : games_) {
    if (existing_id == game_id) {
      checkpoint.games.push_back(
          GameRecord{game_id, candidate.revision, candidate.state});
      replaced = true;
      continue;
    }

    const RunningGame materialized = materialized_game(game, now);
    checkpoint.games.push_back(GameRecord{
        existing_id, materialized.revision, materialized.state});
  }
  if (!replaced) {
    checkpoint.games.push_back(
        GameRecord{game_id, candidate.revision, candidate.state});
  }
  std::sort(checkpoint.games.begin(), checkpoint.games.end(),
      [](const GameRecord& left, const GameRecord& right) {
        return left.game_id < right.game_id;
      });
  return checkpoint;
}

void GameRuntime::commit(
    const std::string& game_id,
    RunningGame candidate,
    Clock::time_point now) {
  if (state_file_ != nullptr) {
    state_file_->save(checkpoint_with(game_id, candidate, now));
  }
  games_.insert_or_assign(game_id, std::move(candidate));
}

}  // namespace minesweeper
