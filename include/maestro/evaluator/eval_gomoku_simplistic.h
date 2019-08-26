#pragma once
#include "../game/game_gomoku.h"
#include <random>

namespace Maestro {
    class SimplisticGomokuEvaluator final : public IEvaluator<Gomoku> {
    public:
        Evaluation<Gomoku> evaluate(const Gomoku& game);
    private:
        void set_check_interval(int d, int& start, int& end);
        int check_dir(const Gomoku::HalfBoard& hb, int dr, int dc);
        void judge(const Gomoku::HalfBoard& b, const Gomoku::HalfBoard& w, int& max_b, int& max_w);
    };
}
