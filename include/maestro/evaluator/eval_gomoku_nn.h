#pragma once
#include "../game/game_gomoku.h"

namespace Maestro {
    class NNGomokuEvaluator final : public IEvaluator<Gomoku> {
    public:
        Evaluation<Gomoku> evaluate(const Gomoku& game) { 
            // TODO @wsh
            return Evaluation<Gomoku>(); 
        }
    };
}
