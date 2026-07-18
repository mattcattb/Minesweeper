
#pragma once

#include <chrono>

#include "core/Board.h"

enum class GameStatus {
  Playing,
  Won,
  Lost
};


class Game {
public:
  Game(int rows, int cols, int mines);

  bool reveal(int row, int col);
  bool toggle_flag(int row, int col);
  void restart();
  void set_paused(bool paused);
  void toggle_paused();

  const Board& board() const { return board_; }
  GameStatus status() const { return status_; }
  bool paused() const { return paused_; }
  int elapsed_seconds() const;

private:
  using Clock = std::chrono::steady_clock;

  void update_status();
  Clock::time_point elapsed_until() const;

  Board board_;
  GameStatus status_ = GameStatus::Playing;
  bool paused_ = false;
  Clock::time_point started_at_;
  Clock::time_point paused_at_;
  Clock::time_point finished_at_;
  Clock::duration paused_duration_ = Clock::duration::zero();
};
