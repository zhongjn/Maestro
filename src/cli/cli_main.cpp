#include <iostream>
#include "../search/search_graph.h"
#include "../game/game_gomoku.h"
#include "../evaluator/eval_gomoku_nn.h"
#include "../play/match.h"
#include "../util/common.h"

using namespace Maestro;
using namespace std;

static void fast_match() {
    // auto eval = make_shared<SimplisticGomokuEvaluator>();
    auto eval = make_shared<NNGomokuEvaluator>("../../../nn/gnn.pt");
    Gomoku g;

    using Config = MonteCarloGraphSearch<Gomoku>::Config;

    Config c1 = Config();
    c1.leaf_batch_count = 16;
    c1.enable_dag = true;
    Config c2 = Config();
    c2.leaf_batch_count = 16;
    //c2.leaf_batch_count = 8;
    c2.enable_dag = false;

    shared_ptr<MonteCarloGraphSearch<Gomoku>> ps1, ps2;

    auto p1_creator = [&]() {
        ps1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps1, 2000);
    };

    auto p2_creator = [&]() {
        ps2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
        return make_shared<MonteCarloAIPlayer<Gomoku>>(ps2, 2000);
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
    c1.leaf_batch_count = 32;
    c1.enable_speculative_evaluation = true;
    //c1.enable_dag = false;
    //c1.same_response = false;

    Config c2 = Config();
    c2.leaf_batch_count = 32;
    c2.enable_speculative_evaluation = true;
    //c2.enable_dag = true;
    //c2.same_response = false;

    auto p1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
    auto ps1 = make_shared<MonteCarloAIPlayer<Gomoku>>(p1, 8000);

    auto p2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
    auto ps2 = make_shared<MonteCarloAIPlayer<Gomoku>>(p2, 16000);

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

static void human_play() {
    auto eval = make_shared<NNGomokuEvaluator>("../../../nn/gnn.pt");

    auto ps1 = make_shared<GomokuConsolePlayer>();

    Gomoku g;
    using Config = MonteCarloGraphSearch<Gomoku>::Config;
    Config c2 = Config();
    c2.leaf_batch_count = 32;
    c2.enable_dag = false;
    c2.enable_speculative_evaluation = true;
    auto p2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
    auto ps2 = make_shared<MonteCarloAIPlayer<Gomoku>>(p2, 100000);

    Round<Gomoku> round(g, move(ps2), move(ps1), false);

    while (true) {
        puts(round.game().to_string().c_str());
        round.step();
        printf("AI stat:\n");
        p2->print_stat();
    }
}

int main() {
    fast_match();
    //human_play();
    //slow_round();
    return 0;
}