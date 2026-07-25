#include "game_state_file.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace minesweeper {

namespace {

using json = nlohmann::json;

constexpr const char* kCheckpointFormat = "minesweeper-checkpoint";
constexpr std::uint32_t kCheckpointVersion = 1;
constexpr std::uintmax_t kMaximumCheckpointBytes = 64 * 1024 * 1024;
constexpr std::size_t kMaximumGames = 10'000;
constexpr int kMaximumRows = 100;
constexpr int kMaximumCols = 100;

std::string status_name(minesweeper::GameStatus status) {
  switch (status) {
    case minesweeper::GameStatus::Playing:
      return "playing";
    case minesweeper::GameStatus::Won:
      return "won";
    case minesweeper::GameStatus::Lost:
      return "lost";
  }
  throw std::invalid_argument("Unknown game status");
}

minesweeper::GameStatus decode_status(const json& value) {
  const std::string status = value.get<std::string>();
  if (status == "playing") {
    return minesweeper::GameStatus::Playing;
  }
  if (status == "won") {
    return minesweeper::GameStatus::Won;
  }
  if (status == "lost") {
    return minesweeper::GameStatus::Lost;
  }
  throw std::invalid_argument("Unknown saved game status: " + status);
}

json encode_tile(const minesweeper::Tile& tile) {
  return {
      {"mine", tile.mine},
      {"revealed", tile.revealed},
      {"flagged", tile.flagged},
  };
}

json encode_board(const minesweeper::Board& board) {
  json tiles = json::array();
  for (const minesweeper::Tile& tile : board.tiles) {
    tiles.push_back(encode_tile(tile));
  }
  return {
      {"rows", board.rows},
      {"cols", board.cols},
      {"tiles", std::move(tiles)},
  };
}

json encode_game(const minesweeper::GameState& game) {
  return {
      {"board", encode_board(game.board)},
      {"status", status_name(game.status)},
      {"elapsedMilliseconds", game.elapsed_milliseconds},
      {"paused", game.paused},
  };
}

json encode_record(const GameRecord& game) {
  return {
      {"gameId", game.game_id},
      {"revision", game.revision},
      {"state", encode_game(game.state)},
  };
}

json encode_checkpoint(const GameCheckpoint& checkpoint) {
  json games = json::array();
  for (const GameRecord& game : checkpoint.games) {
    games.push_back(encode_record(game));
  }
  return {
      {"format", kCheckpointFormat},
      {"version", kCheckpointVersion},
      {"games", std::move(games)},
  };
}

minesweeper::Tile decode_tile(const json& value) {
  return {
      value.at("mine").get<bool>(),
      value.at("revealed").get<bool>(),
      value.at("flagged").get<bool>(),
  };
}

minesweeper::Board decode_board(const json& value) {
  const int rows = value.at("rows").get<int>();
  const int cols = value.at("cols").get<int>();
  if (rows <= 0 || rows > kMaximumRows || cols <= 0 || cols > kMaximumCols) {
    throw std::invalid_argument("Saved board dimensions are outside limits");
  }

  const json& encoded_tiles = value.at("tiles");
  if (!encoded_tiles.is_array()) {
    throw std::invalid_argument("Saved board tiles must be an array");
  }

  const std::size_t expected_tiles =
      static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
  if (encoded_tiles.size() != expected_tiles) {
    throw std::invalid_argument("Saved tile count does not match dimensions");
  }

  std::vector<minesweeper::Tile> tiles;
  tiles.reserve(expected_tiles);
  for (const json& tile : encoded_tiles) {
    tiles.push_back(decode_tile(tile));
  }
  return {rows, cols, std::move(tiles)};
}

minesweeper::GameState decode_game(const json& value) {
  return {
      decode_board(value.at("board")),
      decode_status(value.at("status")),
      value.at("elapsedMilliseconds").get<std::int64_t>(),
      value.at("paused").get<bool>(),
  };
}

GameRecord decode_record(const json& value) {
  return {
      value.at("gameId").get<std::string>(),
      value.at("revision").get<std::uint64_t>(),
      decode_game(value.at("state")),
  };
}

GameCheckpoint decode_checkpoint(const json& value) {
  if (!value.is_object() ||
      value.at("format").get<std::string>() != kCheckpointFormat) {
    throw std::invalid_argument("File is not a Minesweeper checkpoint");
  }
  if (value.at("version").get<std::uint32_t>() != kCheckpointVersion) {
    throw std::invalid_argument("Unsupported Minesweeper checkpoint version");
  }

  const json& encoded_games = value.at("games");
  if (!encoded_games.is_array() || encoded_games.size() > kMaximumGames) {
    throw std::invalid_argument("Saved game collection is outside limits");
  }

  GameCheckpoint checkpoint;
  checkpoint.games.reserve(encoded_games.size());
  for (const json& game : encoded_games) {
    checkpoint.games.push_back(decode_record(game));
  }
  return checkpoint;
}

}  // namespace

GameStateFile::GameStateFile(std::filesystem::path path)
    : path_(std::move(path)) {}

std::optional<GameCheckpoint> GameStateFile::load() const {
  std::error_code error;
  const bool exists = std::filesystem::exists(path_, error);
  if (error) {
    throw std::runtime_error(
        "Unable to inspect game checkpoint " + path_.string() + ": " +
        error.message());
  }
  if (!exists) {
    return std::nullopt;
  }

  const std::uintmax_t size = std::filesystem::file_size(path_, error);
  if (error) {
    throw std::runtime_error(
        "Unable to read game checkpoint size " + path_.string() + ": " +
        error.message());
  }
  if (size == 0 || size > kMaximumCheckpointBytes ||
      size > static_cast<std::uintmax_t>(
          std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error("Game checkpoint size is invalid");
  }

  std::ifstream file(path_, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open game checkpoint " + path_.string());
  }

  std::string contents(static_cast<std::size_t>(size), '\0');
  file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
  if (!file || file.gcount() != static_cast<std::streamsize>(contents.size())) {
    throw std::runtime_error("Unable to read complete game checkpoint");
  }

  try {
    return decode_checkpoint(json::parse(contents));
  } catch (const std::exception& parse_error) {
    throw std::runtime_error(
        "Unable to decode game checkpoint " + path_.string() + ": " +
        parse_error.what());
  }
}

void GameStateFile::save(const GameCheckpoint& checkpoint) const {
  const std::filesystem::path parent = path_.parent_path();
  std::error_code error;
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, error);
    if (error) {
      throw std::runtime_error(
          "Unable to create game checkpoint directory: " + error.message());
    }
  }

  const std::string contents = encode_checkpoint(checkpoint).dump(2);
  if (contents.size() > kMaximumCheckpointBytes) {
    throw std::runtime_error("Game checkpoint exceeds maximum size");
  }

  const std::filesystem::path temporary = path_.string() + ".tmp";
  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
      throw std::runtime_error(
          "Unable to open temporary game checkpoint " + temporary.string());
    }

    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    file.flush();
    if (!file.good()) {
      file.close();
      std::filesystem::remove(temporary, error);
      throw std::runtime_error("Unable to write complete game checkpoint");
    }
  }

  std::filesystem::rename(temporary, path_, error);
  if (error) {
    const std::string rename_error = error.message();
    std::error_code remove_error;
    std::filesystem::remove(temporary, remove_error);
    throw std::runtime_error(
        "Unable to replace game checkpoint " + path_.string() + ": " +
        rename_error);
  }
}

}  // namespace minesweeper
