#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct Score {
  std::string name;
  int seconds;
};

class Leaderboard {
public:
  explicit Leaderboard(std::string file_path);

  void add_score(std::string name, int seconds);
  bool load();
  bool save() const;
  const std::vector<Score>& scores() const { return scores_; }

private:
  void sort_and_trim();

  static constexpr std::size_t kMaximumScores = 40;
  std::string file_path_;
  std::vector<Score> scores_;
};
