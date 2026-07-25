#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <variant>

#include "game_state_file.hpp"
#include "runtime.hpp"

int main() {
  using namespace minesweeper;

  const auto unique = std::chrono::steady_clock::now()
      .time_since_epoch().count();
  const std::filesystem::path directory =
      std::filesystem::temp_directory_path() /
      ("minesweeper-checkpoint-test-" + std::to_string(unique));
  const std::filesystem::path path = directory / "games.snapshot.json";
  GameStateFile state_file(path);
  assert(!state_file.load().has_value());

  {
    GameRuntime runtime(&state_file);
    const Result created =
        runtime.execute("saved-game", CreateGame{4, 4, 3});
    assert(std::holds_alternative<GameSnapshot>(created.response));

    const Result flagged =
        runtime.execute("saved-game", ToggleFlag{{0, 0}});
    assert(std::get<GameSnapshot>(flagged.response).revision == 1);
  }

  const auto loaded = state_file.load();
  assert(loaded.has_value());
  assert(loaded->games.size() == 1);
  assert(loaded->games.front().game_id == "saved-game");
  assert(loaded->games.front().revision == 1);
  assert(tile_at(loaded->games.front().state.board, {0, 0}).flagged);

  int saved_mines = 0;
  for (const Tile& tile : loaded->games.front().state.board.tiles) {
    saved_mines += tile.mine ? 1 : 0;
  }
  assert(saved_mines == 3);

  GameRuntime restored(&state_file);
  restored.restore(*loaded);
  assert(restored.has_game("saved-game"));
  const Result restored_result =
      restored.execute("saved-game", RequestSnapshot{});
  const GameSnapshot& update =
      std::get<GameSnapshot>(restored_result.response);
  assert(update.revision == 1);
  assert(std::get<HiddenTileView>(update.tiles.front()).flagged);

  {
    std::ofstream corrupt(path, std::ios::binary | std::ios::trunc);
    corrupt << "{not valid json";
  }
  bool rejected_corrupt_file = false;
  try {
    static_cast<void>(state_file.load());
  } catch (const std::runtime_error&) {
    rejected_corrupt_file = true;
  }
  assert(rejected_corrupt_file);

  std::error_code cleanup_error;
  std::filesystem::remove_all(directory, cleanup_error);
  assert(!cleanup_error);
}
