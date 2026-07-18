#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>

#include "server/server.h"

namespace {

void print_event(const ServerEvent& event) {
  std::visit([](const auto& value) {
    using Event = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<Event, GameSnapshot>) {
      std::cout << "game=" << value.game_id
                << " revision=" << value.revision
                << " elapsed=" << value.elapsed_seconds
                << " remaining-mines=" << value.remaining_mines
                << '\n';
    } else {
      std::cout << "error request=" << value.request_id
                << " message=" << value.message << '\n';
    }
  }, event);
}

void process_and_print(Server& server) {
  server.process_one();

  ServerEvent event;
  while (server.try_pop_event(event)) {
    print_event(event);
  }
}

}  // namespace

int main() {
  Server server;
  unsigned long request_number = 1;

  server.enqueue(CreateGame{"default", "create-1", 16, 25, 50});
  process_and_print(server);

  std::cout << "Minesweeper runtime ready. Commands: reveal ROW COL, "
               "flag ROW COL, restart, quit\n";

  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string command;
    input >> command;

    std::string request_id = "cli-" + std::to_string(request_number++);
    if (command == "quit") {
      break;
    }

    if (command == "restart") {
      server.enqueue(RestartGame{"cli", "default", request_id});
      process_and_print(server);
      continue;
    }

    int row;
    int col;
    if ((command == "reveal" || command == "flag") && input >> row >> col) {
      if (command == "reveal") {
        server.enqueue(RevealTile{"cli", "default", request_id, row, col});
      } else {
        server.enqueue(ToggleFlag{"cli", "default", request_id, row, col});
      }
      process_and_print(server);
      continue;
    }

    std::cout << "Unknown command\n";
  }

  server.stop();
  return 0;
}
