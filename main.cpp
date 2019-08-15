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
                make_unique<MonteCarloTreeSearch<Gomoku>>(new Gomoku(), 2, &eval)), 1000);

    auto pb = 
        make_unique<MonteCarloAIPlayer<Gomoku>>(
            cast_unique<IMonteCarloSearch<Gomoku>>(
                make_unique<MonteCarloTreeSearch<Gomoku>>(new Gomoku(), 2, &eval)), 1000);

    Round<Gomoku> round(move(pa), move(pb));

    while (true) {
        puts(round.game().to_string().c_str());
        getchar();
        round.step();
    }

    return 0;
}