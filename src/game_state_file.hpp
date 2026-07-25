#pragma once

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game.hpp"

namespace minesweeper {

struct GameRecord {
  std::string game_id;
  std::uint64_t revision = 0;
  minesweeper::GameState state;
};

struct GameCheckpoint {
  std::vector<GameRecord> games;
};

class GameStateFile {
public:
  explicit GameStateFile(std::filesystem::path path);

  std::optional<GameCheckpoint> load() const;
  void save(const GameCheckpoint& checkpoint) const;

private:
  std::filesystem::path path_;
};

}  // namespace minesweeper
