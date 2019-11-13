#include <iostream>
#include "../search/search_graph.h"
#include "../game/game_gomoku.h"
#include "../evaluator/eval_gomoku_nn.h"
#include "../play/match.h"
#include "../util/common.h"
using namespace Maestro;

int main(int argc, char** argv)
{
    assert(argc > 2);
    int nsim = atoi(argv[1]);
    auto eval = make_shared<NNGomokuEvaluator>(argv[2]);
    Gomoku g;
    using Config = MonteCarloGraphSearch<Gomoku>::Config;

    Config c1 = Config();
    c1.leaf_batch_count = 8;
    c1.enable_dag = false;
    Config c2 = Config();
    c2.leaf_batch_count = 64;
    c2.enable_dag = false;

    shared_ptr<MonteCarloGraphSearch<Gomoku>> ps1, ps2;

    auto p1_creator = [&]() {
        ps1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps1, nsim);
    };

    auto p2_creator = [&]() {
        ps2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps2, nsim);
    };

    Match<Gomoku> match(100, p1_creator, p2_creator);
    match.step_to_end(true, false);

    return 0;
}