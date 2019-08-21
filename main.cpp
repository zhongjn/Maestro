#include <iostream>
#include "search/search_graph.h"
#include "game/game_gomoku.h"
#include "evaluator/evaluator_gomoku.h"
#include "play/match.h"
#include "util/common.h"
using namespace Maestro;
using namespace std;

int main() {
    auto eval = make_shared<SimplisticGomokuEvaluator>();

    Gomoku g;
    //g.black.set(1, 1, true);
    //g.white.set(2, 2, true);
    //g.white.set(3, 3, true);
    //g.white.set(4, 4, true);
    //g.white.set(5, 5, true);

    using Config = MonteCarloGraphSearch<Gomoku>::Config;

    Config c1 = Config();
    c1.leaf_batch_count = 1;
    c1.enable_dag = true;
    Config c2 = Config();
    c2.leaf_batch_count = 1;
    //c2.leaf_batch_count = 8;
    c2.enable_dag = false;

    MonteCarloGraphSearch<Gomoku>* p1, * p2 = nullptr;

    auto p1_creator = [&]() {
        auto gs1 = make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
        p1 = gs1.get();
        return make_unique<MonteCarloAIPlayer<Gomoku>>(move(gs1), 1000); 
    };

    auto p2_creator = [&]() {
        auto gs2 = make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
        p2 = gs2.get();
        return make_unique<MonteCarloAIPlayer<Gomoku>>(move(gs2), 1000); 
    };

    Match<Gomoku> match(100, p1_creator, p2_creator);
    while (match.step()) {
        printf("p1 stat:\n");
        p1->print_stat();
        printf("p2 stat:\n");
        p2->print_stat();
    }

    //auto pa =
    //    make_unique<MonteCarloAIPlayer<Gomoku>>(make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g, c1), 10000);

    //auto pb =
    //    make_unique<MonteCarloAIPlayer<Gomoku>>(make_unique<MonteCarloGraphSearch<Gomoku>>(eval, g, c2), 10000);

    //Round<Gomoku> round(g, move(pa), move(pb));

    //while (true) {
    //    puts(round.game().to_string().c_str());
    //    getchar();
    //    round.step();
    //    auto& stat = MonteCarloGraphSearch<Gomoku>::global_stat;
    //    cout << "sim total: " << stat.sim_total << endl;
    //    cout << "sim transposed: " << stat.sim_use_transposition << endl;
    //    cout << "sim game end: " << stat.sim_game_end << endl;
    //    cout << "tt load factor: " << stat.tt_load_factor << endl;

    //    printf("node evaluated total=%d, used=%d\n", stat.node_evaluated_total, stat.node_evaluated_used);
    //    printf("batch count=%d\n", stat.eval_batch_count);
    //    //MonteCarloGraphSearch::global_stat
    //    // printf("ra: %f\n", pa_ptr->save_ratio());
    //}

    return 0;
}