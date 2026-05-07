#include "core/state.hpp"
#include "player/my_player.hpp"
#include <gtest/gtest.h>
using ttt::game::Sign;
using ttt::my_player::FastBoard;
// Test 1: проверка функции победы
TEST(GomokuLogic, DetectsFiveInARow) {
  FastBoard fb;
  fb.rows = 15;
  fb.cols = 15;
  // Очищаем доску
  for (int i = 0; i < 15; i++)
    for (int j = 0; j < 15; j++)
      fb.grid[i][j] = Sign::NONE;

  // Ставим 5 в ряд по горизонтали
  for (int i = 0; i < 5; i++) {
    fb.make_move(7, i, Sign::X);
  }

  EXPECT_TRUE(check_win(fb, 7, 4, Sign::X));
  EXPECT_FALSE(check_win(fb, 7, 4, Sign::O));
}

// Test 2: проверка инициализации LookUp таблицы
TEST(GomokuEval, LookupTableInitialization) {
  ttt::my_player::init_lookup_table();

  // Проверим паттерн "Открытая тройка" (01110)
  // Индекс в троичной системе: 0*1 + 1*3 + 1*9 + 1*27 + 0*81 = 3 + 9 + 27 = 39
  EXPECT_EQ(ttt::my_player::pointTable[39], 100000);
}

// Test 3: оценка атаки (бот должен видеть угрозу)
TEST(GomokuEval, AttackScoreBasic) {
  ttt::my_player::init_lookup_table();
  FastBoard fb;
  fb.rows = 15;
  fb.cols = 15;
  for (int i = 0; i < 15; i++)
    for (int j = 0; j < 15; j++)
      fb.grid[i][j] = Sign::NONE;

  // создаем ситуацию: у противника 3 камня в ряд
  fb.make_move(5, 5, Sign::O);
  fb.make_move(5, 6, Sign::O);
  fb.make_move(5, 7, Sign::O);

  // оценка в пустой клетке рядом должна быть высокой
  int score = attack_score(fb, Sign::O, 5, 8);
  EXPECT_GT(score, 1000);
}
