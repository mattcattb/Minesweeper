#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "server/leaderboard.h"
#include "server/server.h"
#include "server/socket_server.h"

namespace {

std::string environment_value(const char* name, std::string fallback) {
  const char* value = std::getenv(name);
  return value != nullptr && value[0] != '\0' ? value : std::move(fallback);
}

bool initialize_leaderboard(const std::string& file_path) {
  const std::filesystem::path path(file_path);
  std::error_code error;
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
      std::cerr << "Unable to create leaderboard directory: "
                << error.message() << '\n';
      return false;
    }
  }

  Leaderboard leaderboard(file_path);
  if (std::filesystem::exists(path)) {
    if (!leaderboard.load()) {
      std::cerr << "Unable to load leaderboard from " << file_path << '\n';
      return false;
    }
  } else if (!leaderboard.save()) {
    std::cerr << "Unable to create leaderboard at " << file_path << '\n';
    return false;
  }

  std::cout << "Leaderboard data: " << file_path << '\n';
  return true;
}

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
  const std::string data_directory = environment_value(
      "MINESWEEPER_DATA_DIR", "files");
  const std::string leaderboard_path = environment_value(
      "MINESWEEPER_LEADERBOARD_PATH",
      data_directory + "/leaderboard.csv");
  if (!initialize_leaderboard(leaderboard_path)) {
    return 1;
  }

  Server server;

  if (environment_value("MINESWEEPER_MODE", "cli") == "server") {
    const int port = std::stoi(environment_value("MINESWEEPER_PORT", "7575"));
    if (port <= 0 || port > 65535) {
      std::cerr << "MINESWEEPER_PORT must be between 1 and 65535\n";
      return 1;
    }
    SocketServer socket_server(static_cast<std::uint16_t>(port));
    return socket_server.run() ? 0 : 1;
  }

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
