#include <iostream>
#include <maestro/search/search_graph.h>
#include <maestro/game/game_gomoku.h>
#include <maestro/evaluator/eval_gomoku_simplistic.h>
#include <maestro/play/match.h>
#include <maestro/util/common.h>
#include <tvm/runtime/module.h>
using namespace Maestro;
using namespace std;

int main() {
    auto eval = make_shared<SimplisticGomokuEvaluator>();
    // tvm::runtime::Module::LoadFromFile("");
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

    shared_ptr<MonteCarloGraphSearch<Gomoku>> ps1, ps2;

    auto p1_creator = [&]() {
        ps1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps1, 1000);
    };

    auto p2_creator = [&]() {
        ps2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps2, 1000);
    };

    Match<Gomoku> match(100, p1_creator, p2_creator);
    while (match.step()) {
        printf("p1 stat:\n");
        ps1->print_stat();
        printf("p2 stat:\n");
        ps2->print_stat();
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