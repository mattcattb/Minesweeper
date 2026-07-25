#include <cassert>
#include <stdexcept>
#include <utility>

#include "game.hpp"

int main() {
  using minesweeper::GameStatus;
  using minesweeper::MoveResult;

  auto game = minesweeper::create_game(3, 3, {0, 8});
  minesweeper::validate_game(game);
  assert(minesweeper::contains(game.board, {0, 0}));
  assert(!minesweeper::contains(game.board, {-1, 0}));
  assert(minesweeper::tile_index(game.board, {2, 1}) == 7);
  assert(minesweeper::adjacent_mines(game.board, {1, 1}) == 2);
  assert(minesweeper::remaining_mines(game.board) == 2);

  assert(minesweeper::toggle_flag(game, {0, 1}) == MoveResult::Applied);
  assert(minesweeper::remaining_mines(game.board) == 1);
  assert(minesweeper::reveal(game, {0, 1}) == MoveResult::NotAllowed);
  assert(minesweeper::toggle_flag(game, {0, 1}) == MoveResult::Applied);

  assert(minesweeper::set_paused(game, true) == MoveResult::Applied);
  assert(minesweeper::reveal(game, {1, 1}) == MoveResult::NotAllowed);
  assert(minesweeper::set_paused(game, false) == MoveResult::Applied);

  auto lost = game;
  assert(minesweeper::reveal(lost, {0, 0}) == MoveResult::Applied);
  assert(lost.status == GameStatus::Lost);
  assert(minesweeper::tile_at(lost.board, {0, 0}).revealed);
  assert(minesweeper::tile_at(lost.board, {2, 2}).revealed);
  minesweeper::validate_game(lost);

  auto won = minesweeper::create_game(3, 3, {8});
  assert(minesweeper::reveal(won, {0, 0}) == MoveResult::Applied);
  assert(won.status == GameStatus::Won);
  assert(minesweeper::tile_at(won.board, {2, 2}).flagged);
  minesweeper::validate_game(won);

  auto replacement = minesweeper::create_game(2, 2, {1});
  minesweeper::restart(lost, std::move(replacement.board));
  assert(lost.status == GameStatus::Playing);
  assert(lost.elapsed_milliseconds == 0);
  assert(!lost.paused);
  assert(minesweeper::tile_at(lost.board, {0, 1}).mine);
  minesweeper::validate_game(lost);

  bool rejected_duplicate_mine = false;
  try {
    static_cast<void>(minesweeper::create_game(2, 2, {0, 0}));
  } catch (const std::invalid_argument&) {
    rejected_duplicate_mine = true;
  }
  assert(rejected_duplicate_mine);
}
