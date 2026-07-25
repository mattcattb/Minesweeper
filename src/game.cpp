#include "game.hpp"

#include <algorithm>
#include <deque>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace minesweeper {

GameState create_game(
    int rows,
    int cols,
    const std::vector<std::size_t>& mine_indexes) {
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument("Board dimensions must be positive");
  }

  const auto row_count = static_cast<std::size_t>(rows);
  const auto col_count = static_cast<std::size_t>(cols);
  if (row_count > std::numeric_limits<std::size_t>::max() / col_count) {
    throw std::invalid_argument("Board dimensions are too large");
  }

  const std::size_t tile_count = row_count * col_count;
  if (mine_indexes.size() >= tile_count) {
    throw std::invalid_argument("Board must contain at least one safe tile");
  }

  GameState game{
      Board{rows, cols, std::vector<Tile>(tile_count)},
      GameStatus::Playing,
      0,
      false,
  };

  for (const std::size_t mine : mine_indexes) {
    if (mine >= tile_count) {
      throw std::invalid_argument("Mine index is outside the board");
    }
    if (game.board.tiles[mine].mine) {
      throw std::invalid_argument("Duplicate mine index");
    }
    game.board.tiles[mine].mine = true;
  }

  return game;
}

std::vector<std::size_t> choose_mines(
    int rows,
    int cols,
    int mines,
    std::mt19937& generator) {
  if (rows <= 0 || cols <= 0 || mines < 0) {
    throw std::invalid_argument("Invalid board dimensions or mine count");
  }

  const auto row_count = static_cast<std::size_t>(rows);
  const auto col_count = static_cast<std::size_t>(cols);
  if (row_count > std::numeric_limits<std::size_t>::max() / col_count) {
    throw std::invalid_argument("Board dimensions are too large");
  }

  const std::size_t tile_count = row_count * col_count;
  if (static_cast<std::size_t>(mines) >= tile_count) {
    throw std::invalid_argument("Board must contain at least one safe tile");
  }

  std::vector<std::size_t> indexes(tile_count);
  std::iota(indexes.begin(), indexes.end(), std::size_t{0});
  std::shuffle(indexes.begin(), indexes.end(), generator);
  indexes.resize(static_cast<std::size_t>(mines));
  return indexes;
}

bool contains(
    const Board& board,
    Position position) {
  return position.row >= 0 && position.row < board.rows &&
      position.col >= 0 && position.col < board.cols;
}

std::size_t tile_index(
    const Board& board,
    Position position) {

  if (!contains(board, position)) {
    throw std::out_of_range("position index invalid on board");
  }

  return static_cast<std::size_t>(position.row) *
          static_cast<std::size_t>(board.cols) +
      static_cast<std::size_t>(position.col);
}

Tile& tile_at(
    Board& board,
    Position position) {

  return board.tiles.at(tile_index(board, position));
}

const Tile& tile_at(
    const Board& board,
    Position position) {
  return board.tiles.at(tile_index(board, position));
}

int adjacent_mines(
    const Board& board,
    Position position) {
  if (!contains(board, position)) {
    throw std::out_of_range("position index invalid on board");
  }

  int mines = 0;
  for (int row_offset = -1; row_offset <= 1; row_offset += 1) {
    for (int col_offset = -1; col_offset <= 1; col_offset += 1) {
      if (row_offset == 0 && col_offset == 0) {
        continue;
      }

      const Position neighbor{
          position.row + row_offset,
          position.col + col_offset,
      };
      if (contains(board, neighbor) && tile_at(board, neighbor).mine) {
        mines += 1;
      }
    }
  }
  return mines;
}

int remaining_mines(const Board& board) {
  int mines = 0;
  int flags = 0;
  for (const Tile& tile : board.tiles) {
    mines += tile.mine ? 1 : 0;
    flags += tile.flagged ? 1 : 0;
  }
  return mines - flags;
}

MoveResult reveal(
    GameState& game,
    Position position) {
  if (!contains(game.board, position)) {
    return MoveResult::OutOfBounds;
  }
  if (game.status != GameStatus::Playing || game.paused) {
    return MoveResult::NotAllowed;
  }

  Tile& selected = tile_at(game.board, position);
  if (selected.revealed || selected.flagged) {
    return MoveResult::NotAllowed;
  }

  if (selected.mine) {
    for (Tile& tile : game.board.tiles) {
      if (tile.mine) {
        tile.revealed = true;
        tile.flagged = false;
      }
    }
    game.status = GameStatus::Lost;
    return MoveResult::Applied;
  }

  std::deque<Position> pending;
  pending.push_back(position);
  while (!pending.empty()) {
    const Position current = pending.front();
    pending.pop_front();

    Tile& tile = tile_at(game.board, current);
    if (tile.revealed || tile.flagged || tile.mine) {
      continue;
    }

    tile.revealed = true;
    if (adjacent_mines(game.board, current) != 0) {
      continue;
    }

    for (int row_offset = -1; row_offset <= 1; row_offset += 1) {
      for (int col_offset = -1; col_offset <= 1; col_offset += 1) {
        if (row_offset == 0 && col_offset == 0) {
          continue;
        }

        const Position neighbor{
            current.row + row_offset,
            current.col + col_offset,
        };
        if (!contains(game.board, neighbor)) {
          continue;
        }

        const Tile& neighbor_tile = tile_at(game.board, neighbor);
        if (!neighbor_tile.revealed && !neighbor_tile.flagged &&
            !neighbor_tile.mine) {
          pending.push_back(neighbor);
        }
      }
    }
  }

  const bool won = std::all_of(
      game.board.tiles.begin(),
      game.board.tiles.end(),
      [](const Tile& tile) { return tile.mine || tile.revealed; });
  if (won) {
    game.status = GameStatus::Won;
    for (Tile& tile : game.board.tiles) {
      if (tile.mine) {
        tile.flagged = true;
      }
    }
  }

  return MoveResult::Applied;
}

MoveResult toggle_flag(
    GameState& game,
    Position position) {
  if (!contains(game.board, position)) {
    return MoveResult::OutOfBounds;
  }
  if (game.status != GameStatus::Playing || game.paused) {
    return MoveResult::NotAllowed;
  }

  Tile& tile = tile_at(game.board, position);
  if (tile.revealed) {
    return MoveResult::NotAllowed;
  }

  tile.flagged = !tile.flagged;
  return MoveResult::Applied;
}

MoveResult set_paused(GameState& game, bool paused) {
  if (game.status != GameStatus::Playing) {
    return MoveResult::NotAllowed;
  }
  game.paused = paused;
  return MoveResult::Applied;
}

void restart(
    GameState& game,
    Board replacement) {
  GameState restarted{
      std::move(replacement),
      GameStatus::Playing,
      0,
      false,
  };
  validate_game(restarted);
  game = std::move(restarted);
}

void validate_game(const GameState& game) {
  if (game.board.rows <= 0 || game.board.cols <= 0) {
    throw std::invalid_argument("Board dimensions must be positive");
  }
  if (game.elapsed_milliseconds < 0) {
    throw std::invalid_argument("Elapsed time cannot be negative");
  }
  if (game.status != GameStatus::Playing && game.paused) {
    throw std::invalid_argument("A finished game cannot be paused");
  }

  const auto rows = static_cast<std::size_t>(game.board.rows);
  const auto cols = static_cast<std::size_t>(game.board.cols);
  if (rows > std::numeric_limits<std::size_t>::max() / cols ||
      game.board.tiles.size() != rows * cols) {
    throw std::invalid_argument("Tile count does not match board dimensions");
  }

  bool has_safe_tile = false;
  bool has_revealed_mine = false;
  bool all_safe_tiles_revealed = true;
  for (const Tile& tile : game.board.tiles) {
    if (tile.revealed && tile.flagged) {
      throw std::invalid_argument("A tile cannot be revealed and flagged");
    }
    if (tile.mine) {
      has_revealed_mine = has_revealed_mine || tile.revealed;
    } else {
      has_safe_tile = true;
      all_safe_tiles_revealed = all_safe_tiles_revealed && tile.revealed;
    }
  }

  if (!has_safe_tile) {
    throw std::invalid_argument("Board must contain at least one safe tile");
  }
  if (game.status == GameStatus::Playing &&
      (has_revealed_mine || all_safe_tiles_revealed)) {
    throw std::invalid_argument("Playing status does not match tile state");
  }
  if (game.status == GameStatus::Won &&
      (has_revealed_mine || !all_safe_tiles_revealed)) {
    throw std::invalid_argument("Won status does not match tile state");
  }
  if (game.status == GameStatus::Lost && !has_revealed_mine) {
    throw std::invalid_argument("Lost status does not match tile state");
  }
}

}  // namespace minesweeper
