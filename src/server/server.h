#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "core/Game.h"

constexpr std::uint16_t kMinesweeperProtocolVersion = 1;

struct CreateGame {
  std::string game_id;
  std::string request_id;
  int rows;
  int cols;
  int mines;
};

struct RevealTile {
  std::string connection_id;
  std::string game_id;
  std::string request_id;
  int row;
  int col;
};

struct ToggleFlag {
  std::string connection_id;
  std::string game_id;
  std::string request_id;
  int row;
  int col;
};

struct RestartGame {
  std::string connection_id;
  std::string game_id;
  std::string request_id;
};

struct RequestSnapshot {
  std::string connection_id;
  std::string game_id;
  std::string request_id;
};

using Command = std::variant<
    CreateGame,
    RevealTile,
    ToggleFlag,
    RestartGame,
    RequestSnapshot>;

struct HiddenTileView {
  int row;
  int col;
  bool flagged;
};

struct RevealedTileView {
  int row;
  int col;
  bool mine;
  int adjacent_mines;
};

using TileView = std::variant<HiddenTileView, RevealedTileView>;

enum class EventAudience {
  Game,
  Connection,
};

struct GameSnapshot {
  EventAudience audience;
  std::string connection_id;
  std::string game_id;
  std::string request_id;
  std::uint64_t revision;
  GameStatus status;
  int elapsed_seconds;
  int remaining_mines;
  int rows;
  int cols;
  std::vector<TileView> tiles;
};

enum class CommandErrorCode {
  BadRequest,
  GameNotFound,
  InvalidAction,
};

struct CommandRejected {
  std::string connection_id;
  std::string request_id;
  CommandErrorCode code;
  std::string message;
};

using ServerEvent = std::variant<GameSnapshot, CommandRejected>;

class Server {
public:
  void enqueue(Command command);
  bool process_one();
  void run();
  void stop();
  bool try_pop_event(ServerEvent& event);

private:
  struct RunningGame {
    Game game;
    std::uint64_t revision = 0;

    RunningGame(int rows, int cols, int mines) : game(rows, cols, mines) {}
  };

  void handle(const CreateGame& command);
  void handle(const RevealTile& command);
  void handle(const ToggleFlag& command);
  void handle(const RestartGame& command);
  void handle(const RequestSnapshot& command);
  void publish_snapshot(
      EventAudience audience,
      const std::string& connection_id,
      const std::string& game_id,
      const std::string& request_id,
      const RunningGame& game);
  void reject(
      const std::string& connection_id,
      const std::string& request_id,
      CommandErrorCode code,
      const std::string& message);
  RunningGame* find_game(const std::string& game_id);

  std::mutex command_mutex_;
  std::condition_variable command_ready_;
  std::deque<Command> commands_;
  bool running_ = true;

  std::mutex event_mutex_;
  std::deque<ServerEvent> events_;

  std::unordered_map<std::string, std::unique_ptr<RunningGame>> games_;
};
