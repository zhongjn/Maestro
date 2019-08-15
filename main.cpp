#include <iostream>
#include "search/search_graph.h"
#include "game/game_gomoku.h"
#include "evaluator/evaluator_gomoku.h"
#include "play/round.h"
#include "util/common.h"
using namespace Maestro;
using namespace std;

int main() {
    auto eval = make_shared<SimplisticGomokuEvaluator>();

    Gomoku g;
    g.black.set(1, 1, true);
    g.black.set(2, 2, true);
    g.black.set(3, 3, true);
   // g.black.set(4, 4, true);

    auto pa_s = make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g);
    auto* pa_ptr = pa_s.get();

    auto pa = 
        make_unique<MonteCarloAIPlayer<Gomoku>>(
            cast_unique<IMonteCarloSearch<Gomoku>>(
                move(pa_s)), 10000);

    auto pb = 
        make_unique<MonteCarloAIPlayer<Gomoku>>(
            cast_unique<IMonteCarloSearch<Gomoku>>(
                make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g)), 10000);
    
    Round<Gomoku> round(g, move(pa), move(pb));

    while (true) {
        puts(round.game().to_string().c_str());
        getchar();
        round.step();
        printf("ra: %f\n", pa_ptr->save_ratio());
    }

    return 0;
}