#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "game.hpp"
#include "game_state_file.hpp"

namespace minesweeper {

struct CreateGame {
  int rows;
  int cols;
  int mines;
};

struct RevealTile {
  minesweeper::Position position;
};

struct ToggleFlag {
  minesweeper::Position position;
};

struct RestartGame {};
struct RequestSnapshot {};

using Command = std::variant<
    CreateGame,
    RevealTile,
    ToggleFlag,
    RestartGame,
    RequestSnapshot>;

enum class EventAudience {
  Game,
  Connection,
};

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

struct GameSnapshot {
  std::uint64_t revision;
  minesweeper::GameStatus status;
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

struct CommandError {
  CommandErrorCode code;
  std::string message;
};

using Response = std::variant<GameSnapshot, CommandError>;

struct Result {
  EventAudience audience;
  Response response;
};

class GameRuntime {
public:
  explicit GameRuntime(GameStateFile* state_file = nullptr)
      : state_file_(state_file) {}

  Result execute(const std::string& game_id, const Command& command);
  bool has_game(const std::string& game_id) const;
  GameCheckpoint checkpoint() const;
  void restore(const GameCheckpoint& checkpoint);

private:
  using Clock = std::chrono::steady_clock;

  struct RunningGame {
    std::uint64_t revision = 0;
    minesweeper::GameState state;
    Clock::time_point updated_at = Clock::now();
  };

  Result handle(const std::string& game_id, const CreateGame& command);
  Result handle(const std::string& game_id, const RevealTile& command);
  Result handle(const std::string& game_id, const ToggleFlag& command);
  Result handle(const std::string& game_id, const RestartGame& command);
  Result handle(const std::string& game_id, const RequestSnapshot& command);

  RunningGame* find_game(const std::string& game_id);
  RunningGame materialized_game(
      const RunningGame& game,
      Clock::time_point now) const;
  GameSnapshot snapshot(const RunningGame& game) const;
  GameCheckpoint checkpoint_at(Clock::time_point now) const;
  GameCheckpoint checkpoint_with(
      const std::string& game_id,
      const RunningGame& candidate,
      Clock::time_point now) const;
  void commit(
      const std::string& game_id,
      RunningGame candidate,
      Clock::time_point now);

  std::unordered_map<std::string, RunningGame> games_;
  std::mt19937 random_generator_{std::random_device{}()};
  GameStateFile* state_file_;
};

}  // namespace minesweeper
