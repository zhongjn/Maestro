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
    //g.black.set(1, 1, true);
    //g.black.set(2, 2, true);
    //g.black.set(3, 3, true);
    //g.black.set(4, 4, true);

    auto pa =
        make_unique<MonteCarloAIPlayer<Gomoku>>(make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g), 2000);

    auto pb =
        make_unique<MonteCarloAIPlayer<Gomoku>>(make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g), 2000);

    Round<Gomoku> round(g, move(pa), move(pb));

    while (true) {
        puts(round.game().to_string().c_str());
        getchar();
        round.step();
        auto& stat = MonteCarloGraphSearch<Gomoku>::global_stat;
        cout << "sim total: " << stat.sim_total << endl;
        cout << "sim transposed: " << stat.sim_use_transposition << endl;
        cout << "tt load factor: " << stat.tt_load_factor << endl;

        {
            const int block = 4;
            int used = 0;
            int unused = 0;
            cout << "children evaluated distribution:" << endl;
            for (auto& kvp : stat.children_evaluated_dist) {
                cout << "{" << kvp.first << ":" << kvp.second << "} ";
                int up = (kvp.first + block - 1) / block * block;
                int redundant = up - kvp.first;
                unused += redundant * kvp.second;
                used += kvp.second;
            }
            cout << endl;
            printf("with blocking factor %d, used=%d, unused=%d\n", block, used, unused);
        }
        //MonteCarloGraphSearch::global_stat
        // printf("ra: %f\n", pa_ptr->save_ratio());
    }

    return 0;
}