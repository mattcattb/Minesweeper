#include "leaderboard.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace minesweeper {

Leaderboard::Leaderboard(std::string file_path)
    : file_path_(std::move(file_path)) {}

void Leaderboard::add_score(std::string name, int seconds) {
  scores_.push_back({std::move(name), seconds});
  sort_and_trim();
}

bool Leaderboard::load() {
  std::ifstream file(file_path_);
  if (!file.is_open()) {
    return false;
  }

  std::vector<Score> loaded_scores;
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream stream(line);
    std::string seconds_text;
    std::string name;

    if (!std::getline(stream, seconds_text, ',') ||
        !std::getline(stream, name) || name.empty()) {
      continue;
    }

    try {
      int seconds = std::stoi(seconds_text);
      if (seconds >= 0) {
        loaded_scores.push_back({std::move(name), seconds});
      }
    } catch (...) {
      // Ignore malformed rows and continue loading valid scores.
    }
  }

  scores_ = std::move(loaded_scores);
  sort_and_trim();
  return true;
}

bool Leaderboard::save() const {
  std::ofstream file(file_path_);
  if (!file.is_open()) {
    return false;
  }

  for (const Score& score : scores_) {
    file << score.seconds << ',' << score.name << '\n';
  }
  return file.good();
}

void Leaderboard::sort_and_trim() {
  std::sort(scores_.begin(), scores_.end(), [](const Score& left, const Score& right) {
    return left.seconds < right.seconds;
  });

  if (scores_.size() > kMaximumScores) {
    scores_.resize(kMaximumScores);
  }
}

}  // namespace minesweeper
