#include <cassert>
#include <iostream>
#include <variant>

#include "server/server.h"

int main() {
  Server server;
  ServerEvent event;

  server.enqueue(CreateGame{"test-game", "create", 4, 4, 3});
  assert(server.process_one());
  assert(server.try_pop_event(event));
  assert(std::holds_alternative<GameSnapshot>(event));

  const GameSnapshot& created = std::get<GameSnapshot>(event);
  assert(created.revision == 0);
  assert(created.rows == 4);
  assert(created.cols == 4);
  assert(created.tiles.size() == 16);
  for (const TileView& tile : created.tiles) {
    assert(std::holds_alternative<HiddenTileView>(tile));
  }

  server.enqueue(ToggleFlag{"test", "test-game", "flag", 0, 0});
  assert(server.process_one());
  assert(server.try_pop_event(event));
  assert(std::holds_alternative<GameSnapshot>(event));

  const GameSnapshot& flagged = std::get<GameSnapshot>(event);
  assert(flagged.revision == 1);
  assert(std::get<HiddenTileView>(flagged.tiles.front()).flagged);

  server.enqueue(RevealTile{"test", "test-game", "invalid", 99, 99});
  assert(server.process_one());
  assert(server.try_pop_event(event));
  assert(std::holds_alternative<CommandRejected>(event));
  assert(std::get<CommandRejected>(event).code == CommandErrorCode::BadRequest);

  server.enqueue(RequestSnapshot{"test", "test-game", "resync"});
  assert(server.process_one());
  assert(server.try_pop_event(event));
  assert(std::holds_alternative<GameSnapshot>(event));
  assert(std::get<GameSnapshot>(event).audience == EventAudience::Connection);

  server.enqueue(RestartGame{"test", "test-game", "restart"});
  assert(server.process_one());
  assert(server.try_pop_event(event));
  assert(std::holds_alternative<GameSnapshot>(event));

  const GameSnapshot& restarted = std::get<GameSnapshot>(event);
  assert(restarted.revision == 2);
  assert(restarted.remaining_mines == 3);
  for (const TileView& tile : restarted.tiles) {
    assert(std::holds_alternative<HiddenTileView>(tile));
    assert(!std::get<HiddenTileView>(tile).flagged);
  }

  std::cout << "server tests passed\n";
  return 0;
}
