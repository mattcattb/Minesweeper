#include <cassert>
#include <variant>

#include "runtime.hpp"

int main() {
  using namespace minesweeper;

  GameRuntime runtime;
  const Result created_result =
      runtime.execute("test-game", CreateGame{4, 4, 3});
  assert(created_result.audience == EventAudience::Game);
  assert(std::holds_alternative<GameSnapshot>(created_result.response));

  const GameSnapshot& created =
      std::get<GameSnapshot>(created_result.response);
  assert(created.revision == 0);
  assert(created.rows == 4);
  assert(created.cols == 4);
  assert(created.tiles.size() == 16);
  for (const TileView& tile : created.tiles) {
    assert(std::holds_alternative<HiddenTileView>(tile));
  }

  const Result flagged_result =
      runtime.execute("test-game", ToggleFlag{{0, 0}});
  const GameSnapshot& flagged =
      std::get<GameSnapshot>(flagged_result.response);
  assert(flagged.revision == 1);
  assert(std::get<HiddenTileView>(flagged.tiles.front()).flagged);

  const Result invalid =
      runtime.execute("test-game", RevealTile{{99, 99}});
  assert(std::holds_alternative<CommandError>(invalid.response));
  assert(std::get<CommandError>(invalid.response).code ==
      CommandErrorCode::BadRequest);

  const Result snapshot =
      runtime.execute("test-game", RequestSnapshot{});
  assert(snapshot.audience == EventAudience::Connection);
  assert(std::holds_alternative<GameSnapshot>(snapshot.response));

  const Result restarted_result =
      runtime.execute("test-game", RestartGame{});
  const GameSnapshot& restarted =
      std::get<GameSnapshot>(restarted_result.response);
  assert(restarted.revision == 2);
  assert(restarted.remaining_mines == 3);
  for (const TileView& tile : restarted.tiles) {
    assert(std::holds_alternative<HiddenTileView>(tile));
    assert(!std::get<HiddenTileView>(tile).flagged);
  }
}
