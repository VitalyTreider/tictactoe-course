#include "my_player.hpp"
#include <algorithm>
#include <vector>
namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
  init_lookup_table();
  if (state.get_move_no() == 0)
    return find_start_move(state);

  FastBoard fb;
  fb.sync(state);

  Point best_move;
  Sign opponent = (m_sign == Sign::X) ? Sign::O : Sign::X;

  std::vector<RatedMove> root_moves;
  for (int x = 0; x < fb.rows; x++) {
    for (int y = 0; y < fb.cols; y++) {
      if (fb.get(x, y) != Sign::NONE || !is_promising(fb, x, y))
        continue;

      int a_score = attack_score(fb, m_sign, x, y);
      int d_score = attack_score(fb, opponent, x, y);

      // победа в один ход
      if (a_score >= 10000000) {
        best_move.x = x;
        best_move.y = y;
        return best_move;
      }

      root_moves.push_back({x, y, a_score + (d_score * 2)});
    }
  }

  std::sort(root_moves.begin(), root_moves.end(), compareMoves);
  if (root_moves.size() > 6)
    root_moves.resize(6);

  if (!root_moves.empty()) {
    best_move.x = root_moves[0].x;
    best_move.y = root_moves[0].y;
  }

  int alpha = -2000000000;
  int beta = 2000000000;
  int max_score = -2000000000;

  for (const auto &m : root_moves) {
    fb.make_move(m.x, m.y, m_sign);
    int score = -negamax(fb, 5, -beta, -alpha, opponent, m_sign, m.x, m.y);
    fb.undo_move(m.x, m.y);

    if (score > max_score) {
      max_score = score;
      best_move.x = m.x;
      best_move.y = m.y;
    }
    // Обновляем alpha на корневом уровне, иначе отсечение не работает
    if (max_score > alpha) {
      alpha = max_score;
    }
  }

  return best_move;
}

}; // namespace ttt::my_player
