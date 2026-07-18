#include "core/Game.h"

Game::Game(int rows, int cols, int mines)
    : board_(rows, cols, mines),
      started_at_(Clock::now()),
      paused_at_(started_at_),
      finished_at_(started_at_) {}

bool Game::reveal(int row, int col) {
  if (status_ != GameStatus::Playing || paused_ || !board_.contains(row, col)) {
    return false;
  }

  bool was_revealed = board_.tile_at(row, col).is_revealed();
  board_.update_board(row, col, true);
  update_status();
  return !was_revealed && board_.tile_at(row, col).is_revealed();
}

bool Game::toggle_flag(int row, int col) {
  if (status_ != GameStatus::Playing || paused_ || !board_.contains(row, col)) {
    return false;
  }

  bool was_flagged = board_.tile_at(row, col).flag_placed();
  board_.update_board(row, col, false);
  update_status();
  return was_flagged != board_.tile_at(row, col).flag_placed();
}

void Game::restart() {
  board_.reset_board();
  status_ = GameStatus::Playing;
  paused_ = false;
  started_at_ = Clock::now();
  paused_at_ = started_at_;
  finished_at_ = started_at_;
  paused_duration_ = Clock::duration::zero();
}

void Game::set_paused(bool paused) {
  if (status_ != GameStatus::Playing || paused_ == paused) {
    return;
  }

  if (paused) {
    paused_at_ = Clock::now();
  } else {
    paused_duration_ += Clock::now() - paused_at_;
  }

  paused_ = paused;
}

void Game::toggle_paused() {
  set_paused(!paused_);
}

int Game::elapsed_seconds() const {
  return std::chrono::duration_cast<std::chrono::seconds>(
      elapsed_until() - started_at_ - paused_duration_).count();
}

void Game::update_status() {
  if (board_.board_state() == 0) {
    return;
  }

  finished_at_ = Clock::now();

  if (board_.board_state() == 1) {
    status_ = GameStatus::Won;
    board_.flag_all_mines();
    return;
  }

  status_ = GameStatus::Lost;
  board_.reveal_mines();
}

Game::Clock::time_point Game::elapsed_until() const {
  if (status_ != GameStatus::Playing) {
    return finished_at_;
  }

  if (paused_) {
    return paused_at_;
  }

  return Clock::now();
}
