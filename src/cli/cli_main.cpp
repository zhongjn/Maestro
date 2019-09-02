#include <iostream>
#include "../search/search_graph.h"
#include "../game/game_gomoku.h"
#include "../evaluator/eval_gomoku_simplistic.h"
#include "../play/match.h"
#include "../util/common.h"

using namespace Maestro;
using namespace std;

static void fast_match() {
    // auto eval = make_shared<SimplisticGomokuEvaluator>();
    auto eval = make_shared<NNGomokuEvaluator>("../../../nn/nn.pt");
    Gomoku g;

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
}

static void slow_round() {
    // auto eval = make_shared<SimplisticGomokuEvaluator>();
    auto eval = make_shared<NNGomokuEvaluator>("../../../nn/gnn.pt");

    Gomoku g;
    using Config = MonteCarloGraphSearch<Gomoku>::Config;


    Config c1 = Config();
    c1.leaf_batch_count = 8;
    c1.enable_speculative_evaluation = true;

    Config c2 = Config();
    c2.leaf_batch_count = 8;
    c2.enable_speculative_evaluation = false;


    auto p1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
    auto ps1 = make_shared<MonteCarloAIPlayer<Gomoku>>(p1, 2000);

    auto p2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
    auto ps2 = make_shared<MonteCarloAIPlayer<Gomoku>>(p2, 2000);

    Round<Gomoku> round(g, move(ps1), move(ps2), true);

    while (true) {
        puts(round.game().to_string().c_str());
        getchar();
        round.step();
        printf("p1 stat:\n");
        p1->print_stat();
        printf("p2 stat:\n");
        p2->print_stat();
    }
}

int main() {
    // fast_match();
    slow_round();
    return 0;
}