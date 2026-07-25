#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace minesweeper {

enum class GameStatus {
  Playing,
  Won,
  Lost,
};

struct Position {
  int row;
  int col;
};

struct Tile {
  bool mine = false;
  bool revealed = false;
  bool flagged = false;
};

struct Board {
  int rows = 0;
  int cols = 0;
  std::vector<Tile> tiles;
};

struct GameState {
  Board board;
  GameStatus status = GameStatus::Playing;
  std::int64_t elapsed_milliseconds = 0;
  bool paused = false;
};

enum class MoveResult {
  Applied,
  OutOfBounds,
  NotAllowed,
};

GameState create_game(
    int rows,
    int cols,
    const std::vector<std::size_t>& mine_indexes);

std::vector<std::size_t> choose_mines(
    int rows,
    int cols,
    int mines,
    std::mt19937& generator);

bool contains(const Board& board, Position position);

std::size_t tile_index(const Board& board, Position position);

Tile& tile_at(Board& board, Position position);

const Tile& tile_at(const Board& board, Position position);

int adjacent_mines(const Board& board, Position position);

int remaining_mines(const Board& board);

MoveResult reveal(GameState& game, Position position);

MoveResult toggle_flag(GameState& game, Position position);

MoveResult set_paused(GameState& game, bool paused);

void restart(GameState& game, Board replacement);

void validate_game(const GameState& game);

}  // namespace minesweeper
