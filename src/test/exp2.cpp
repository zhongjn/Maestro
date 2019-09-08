 #include <iostream>
#include <time.h>
#include "../search/search_graph.h"
#include "../game/game_gomoku.h"
#include "../evaluator/eval_gomoku_nn.h"
#include "../play/match.h"
#include "../util/common.h"
using namespace Maestro;

int main(int argc, char** argv)
{
    assert(argc > 1);
    bool enable_speculative_evaluation = atoi(argv[1]) == 0 ? false : true;

    auto eval = make_shared<NNGomokuEvaluator>(argv[2]);

    Gomoku g;
    using Config = MonteCarloGraphSearch<Gomoku>::Config;
    Config c1 = Config();
    c1.leaf_batch_count = 8;
    c1.enable_speculative_evaluation = enable_speculative_evaluation;
    c1.enable_dag = false;

    Config c2 = Config();
    c2.leaf_batch_count = 8;
    c2.enable_speculative_evaluation = enable_speculative_evaluation;
    c1.enable_dag = false;

    auto p1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
    auto ps1 = make_shared<MonteCarloAIPlayer<Gomoku>>(p1, 10000);

    auto p2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
    auto ps2 = make_shared<MonteCarloAIPlayer<Gomoku>>(p2, 10000);

    Round<Gomoku> round(g, move(ps1), move(ps2), true);


    time_t start, end;
    float max = 0, min = 100000000, sum = 0, dur;
    int nstep = 0;
    while (true) {
        start = clock();
        if (!round.step()) {
            break;
        }
        end = clock();
        dur = ((float)end - start) / CLK_TCK;
        max = max > dur ? max : dur;
        min = min < dur ? min : dur;
        sum += dur;
        ++nstep;
        printf("Step %d\n", nstep);
    }
    if (enable_speculative_evaluation) {
        printf("Enable speculative evaluation:\n");
    } else {
        printf("Disable speculative evaluation:\n");
    }
    printf("max: %f, min: %f, avg: %f\n", max, min, sum / nstep);
    return 0;
}