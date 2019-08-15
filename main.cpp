#include <iostream>
#include "search/search_tree.h"
#include "game/game_gomoku.h"
#include "evaluator/evaluator_gomoku.h"
#include "play/round.h"
#include "util/common.h"
using namespace Maestro;
using namespace std;

int main() {
    SimplisticGomokuEvaluator eval;

    auto pa = 
        make_unique<MonteCarloAIPlayer<Gomoku>>(
            cast_unique<IMonteCarloSearch<Gomoku>>(
                make_unique<MonteCarloTreeSearch<Gomoku>>(new Gomoku(), 2, &eval)), 2);

    auto pb = 
        make_unique<MonteCarloAIPlayer<Gomoku>>(
            cast_unique<IMonteCarloSearch<Gomoku>>(
                make_unique<MonteCarloTreeSearch<Gomoku>>(new Gomoku(), 2, &eval)), 2);

    Round<Gomoku> round(move(pa), move(pb));

    while (true) {
        puts(round.game().to_string().c_str());
        // getchar();
        round.step();
        break;
    }

    return 0;
}