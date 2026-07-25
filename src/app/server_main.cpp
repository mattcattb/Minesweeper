#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "game_state_file.hpp"
#include "leaderboard.hpp"
#include "runtime.hpp"
#include "tcp_server.hpp"

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

  minesweeper::Leaderboard leaderboard(file_path);
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

void print_result(const minesweeper::Result& result) {
  if (const auto* snapshot =
          std::get_if<minesweeper::GameSnapshot>(&result.response)) {
    std::cout << "revision=" << snapshot->revision
              << " elapsed=" << snapshot->elapsed_seconds
              << " remaining-mines=" << snapshot->remaining_mines
              << '\n';
  } else {
    std::cout << "error="
              << std::get<minesweeper::CommandError>(result.response).message
              << '\n';
  }
}

}  // namespace

int main() {
  const std::string data_directory = environment_value(
      "MINESWEEPER_DATA_DIR", "data");
  const std::string leaderboard_path = environment_value(
      "MINESWEEPER_LEADERBOARD_PATH",
      data_directory + "/leaderboard.csv");
  if (!initialize_leaderboard(leaderboard_path)) {
    return 1;
  }

  const std::string game_state_path = environment_value(
      "MINESWEEPER_GAME_STATE_PATH",
      data_directory + "/games.snapshot.json");
  minesweeper::GameStateFile game_state_file(game_state_path);
  minesweeper::GameRuntime runtime(&game_state_file);
  try {
    if (const auto checkpoint = game_state_file.load()) {
      runtime.restore(*checkpoint);
      std::cout << "Game checkpoint: " << game_state_path << '\n';
    }
  } catch (const std::exception& error) {
    std::cerr << "Unable to restore games: " << error.what() << '\n';
    return 1;
  }

  if (environment_value("MINESWEEPER_MODE", "cli") == "server") {
    const int port = std::stoi(environment_value("MINESWEEPER_PORT", "7575"));
    if (port <= 0 || port > 65535) {
      std::cerr << "MINESWEEPER_PORT must be between 1 and 65535\n";
      return 1;
    }
    minesweeper::TcpServer tcp_server(
        static_cast<std::uint16_t>(port), runtime);
    try {
      return tcp_server.run() ? 0 : 1;
    } catch (const std::exception& error) {
      std::cerr << "Minesweeper server stopped: " << error.what() << '\n';
      return 1;
    }
  }

  if (runtime.has_game("default")) {
    print_result(
        runtime.execute("default", minesweeper::RequestSnapshot{}));
  } else {
    print_result(
        runtime.execute("default", minesweeper::CreateGame{16, 25, 50}));
  }

  std::cout << "Minesweeper runtime ready. Commands: reveal ROW COL, "
               "flag ROW COL, restart, quit\n";

  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string command;
    input >> command;

    if (command == "quit") {
      break;
    }

    if (command == "restart") {
      print_result(
          runtime.execute("default", minesweeper::RestartGame{}));
      continue;
    }

    int row;
    int col;
    if ((command == "reveal" || command == "flag") && input >> row >> col) {
      if (command == "reveal") {
        print_result(runtime.execute(
            "default", minesweeper::RevealTile{{row, col}}));
      } else {
        print_result(runtime.execute(
            "default", minesweeper::ToggleFlag{{row, col}}));
      }
      continue;
    }

    std::cout << "Unknown command\n";
  }

  try {
    game_state_file.save(runtime.checkpoint());
  } catch (const std::exception& error) {
    std::cerr << "Unable to save games during shutdown: "
              << error.what() << '\n';
    return 1;
  }
  return 0;
}
