#pragma once

#include "core/game.hpp"
#include "core/state.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

inline int pointTable[243];

struct FastBoard {
  Sign grid[20][20];
  int rows;
  int cols;
  void sync(const State &state) {
    rows = state.get_opts().rows;
    cols = state.get_opts().cols;
    for (int x = 0; x < rows; x++) {
      for (int y = 0; y < cols; y++) {
        grid[x][y] = state.get_value(x, y);
      }
    }
  }
  void make_move(int x, int y, Sign sgn) { grid[x][y] = sgn; }
  void undo_move(int x, int y) { grid[x][y] = Sign::NONE; }
  Sign get(int x, int y) const {
    if (x < 0 || y < 0 || x >= rows || y >= cols)
      return Sign::WALL;
    return grid[x][y];
  }
};

inline bool is_promising(const FastBoard &fb, int x, int y) {
  for (int dx = -2; dx <= 2; dx++) {
    for (int dy = -2; dy <= 2; dy++) {
      if (dx == 0 && dy == 0)
        continue;
      Sign val = fb.get(x + dx, y + dy);
      if (val == Sign::X || val == Sign::O)
        return true;
    }
  }
  return false;
}

inline void build_line(const FastBoard &fb, Sign sgn, int x, int y, int dx,
                       int dy, int line[9]) {
  for (int k = -4; k <= 4; k++) {
    int idx = k + 4;
    if (k == 0) {
      line[idx] = 1;
      continue;
    }

    Sign v = fb.get(x + k * dx, y + k * dy);
    if (v == sgn)
      line[idx] = 1;
    else if (v == Sign::NONE)
      line[idx] = 0;
    else
      line[idx] = 2;
  }
}

inline int get_index(const int window[5]) {
  int index = 0;
  int power = 1;
  for (int i = 0; i < 5; i++) {
    index += window[i] * power;
    power *= 3;
  }
  return index;
}

inline void init_lookup_table() {
  for (int i = 0; i < 243; i++) {
    int temp = i;
    int window[5];
    int stones = 0;
    int blocked = 0;

    for (int j = 0; j < 5; j++) {
      window[j] = temp % 3;
      if (window[j] == 1)
        stones++;
      if (window[j] == 2)
        blocked++;
      temp /= 3;
    }
    if (blocked > 0) {
      pointTable[i] = 0;
    } else {
      if (stones == 5)
        pointTable[i] = 100000000;
      else if (stones == 4) {
        pointTable[i] = 20000;
      } else if (stones == 3) {
        if (window[0] == 0 && window[4] == 0)
          pointTable[i] = 100000;
        else
          pointTable[i] = 5000;
        // pointTable[i] = 1000;
      } else if (stones == 2)
        if (window[0] == 0 && window[4] == 0)
          pointTable[i] = 10000; // Открытая двойка (0110) — это база для тройки
        else
          pointTable[i] = 1000;
      else
        pointTable[i] = 0;
    }
  }
}

inline int score_line_segment(int line[9]) {
  int score = 0;
  for (int i = 0; i <= 4; i++) {
    int index = 0;
    int power = 1;
    for (int j = 0; j < 5; j++) {
      index += line[i + j] * power;
      power *= 3;
    }
    score += pointTable[index];
  }

  for (int i = 0; i <= 3; i++) {
    if (line[i] == 0 && line[i + 1] == 1 && line[i + 2] == 1 &&
        line[i + 3] == 1 && line[i + 4] == 1 && line[i + 5] == 0)
      score += 1000000;
  }
  return score;
}

inline Point find_start_move(const State &state) {
  int rows = state.get_opts().rows;
  int cols = state.get_opts().cols;
  Point p;
  int cx = rows / 2;
  int cy = cols / 2;

  int max_r = (rows > cols) ? rows : cols;
  for (int r = 0; r <= max_r; r++) {
    for (int dx = -r; dx <= r; dx++) {
      for (int dy = -r; dy <= r; dy++) {
        if (abs(dx) != r && abs(dy) != r)
          continue;

        int x = cx + dx;
        int y = cy + dy;

        if (x < 0 || y < 0 || x >= rows || y >= cols)
          continue;
        if (state.get_value(x, y) == Sign::NONE) {
          p.x = x;
          p.y = y;
          return p;
        }
      }
    }
  }
  p.x = 0;
  p.y = 0;
  return p;
}

inline int attack_score(const FastBoard &fb, Sign sgn, int x, int y) {
  int score = 0;
  int threats_count = 0;
  int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  for (auto &d : directions) {
    int line[9];
    build_line(fb, sgn, x, y, d[0], d[1], line);
    // score += score_line_segment(line);
    int s = score_line_segment(line);
    if (s >= 1000)
      threats_count++;
    score += s;
  }
  if (threats_count >= 2) {
    score *= 10;
  }
  return score;
}

inline int evaluate_board(const FastBoard &fb, Sign current_sign) {
  int my_score = 0;
  int opp_score = 0;
  Sign opponent = (current_sign == Sign::X) ? Sign::O : Sign::X;

  for (int x = 0; x < fb.rows; x++) {
    for (int y = 0; y < fb.cols; y++) {
      // Оцениваем только клетки рядом с камнями
      if (fb.get(x, y) == Sign::NONE && is_promising(fb, x, y)) {
        my_score += attack_score(fb, current_sign, x, y);
        opp_score += attack_score(fb, opponent, x, y);
      }
    }
  }
  return my_score - (int)(opp_score * 0.9);
}

struct RatedMove {
  int x, y;
  int weight;
};

inline bool compareMoves(const RatedMove &a, const RatedMove &b) {
  return a.weight > b.weight;
}

inline bool check_win(const FastBoard &fb, int x, int y, Sign sgn) {
  if (x < 0 || y < 0)
    return false;
  int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  for (auto &d : directions) {
    int count = 1;
    // вперед по направлению
    for (int i = 1; i <= 4; i++) {
      if (fb.get(x + i * d[0], y + i * d[1]) == sgn)
        count++;
      else
        break;
    }
    // назад по направлению
    for (int i = 1; i <= 4; i++) {
      if (fb.get(x - i * d[0], y - i * d[1]) == sgn)
        count++;
      else
        break;
    }
    if (count >= 5)
      return true;
  }
  return false;
}

inline int negamax(FastBoard &fb, int depth, int alpha, int beta,
                   Sign current_sign, Sign bot_sign, int last_x, int last_y) {
  Sign opponent = (current_sign == Sign::X) ? Sign::O : Sign::X;

  // если в предыдущем ходе победил противник, ветка проигрышная
  if (check_win(fb, last_x, last_y, opponent)) {
    // return -100000000;
    return -100000000 + (depth * 1000);
  }

  // быстая оценка через максимум
  if (depth == 0) {
    return evaluate_board(fb, current_sign);
  }

  std::vector<RatedMove> moves;
  for (int x = 0; x < fb.rows; x++) {
    for (int y = 0; y < fb.cols; y++) {
      if (fb.get(x, y) != Sign::NONE || !is_promising(fb, x, y))
        continue;

      int a_score = attack_score(fb, current_sign, x, y);
      int d_score = attack_score(fb, opponent, x, y);

      // если ход победный, дальше не копаем
      if (a_score >= 10000000)
        return 100000000;

      moves.push_back({x, y, a_score + d_score});
    }
  }

  if (moves.empty())
    return 0; // ничья (доска заполнена)

  std::sort(moves.begin(), moves.end(), compareMoves);
  if (moves.size() > 6)
    moves.resize(6); // ограничиваем ветвление для скорости

  int max_score = -2000000000;
  for (const auto &m : moves) {
    fb.make_move(m.x, m.y, current_sign);
    int score =
        -negamax(fb, depth - 1, -beta, -alpha, opponent, bot_sign, m.x, m.y);
    fb.undo_move(m.x, m.y);

    if (score > max_score)
      max_score = score;
    if (score > alpha)
      alpha = score;
    if (alpha >= beta)
      break;
  }

  return max_score;
}

class MyPlayer : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;

public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

}; // namespace ttt::my_player
