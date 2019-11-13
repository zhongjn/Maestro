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
    auto eval = make_shared<NNGomokuEvaluator>("../nn/gnn.pt");

    Gomoku g;
    using Config = MonteCarloGraphSearch<Gomoku>::Config;
    Config c1 = Config();
    c1.leaf_batch_count = 8;
    c1.enable_speculative_evaluation = false;
    c1.enable_dag = false;

    Config c2 = Config();
    c2.leaf_batch_count = 8;
    c2.enable_speculative_evaluation = false;
    c1.enable_dag = true;

    auto p1 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c1);
    auto ps1 = make_shared<MonteCarloAIPlayer<Gomoku>>(p1, 2000);

    auto p2 = make_shared<MonteCarloGraphSearch<Gomoku>>(eval, g, c2);
    auto ps2 = make_shared<MonteCarloAIPlayer<Gomoku>>(p2, 2000);

    Round<Gomoku> round(g, move(ps1), move(ps2), true);


    time_t start, end;
    float max_MCTS = 0, max_MCGS = 0, min_MCTS = 100000000, min_MCGS = 100000000, sum_MCTS = 0, sum_MCGS = 0, dur;
    int nstep_MCGS = 0, nstep_MCTS = 0, nstep = 0;
    while (true) {
        start = clock();
        if (!round.step()) {
            break;
        }
        end = clock();
        dur = ((float)end - start) / CLK_TCK;
        if (nstep % 2 == 0) {
            max_MCTS = max_MCTS > dur ? max_MCTS : dur;
            min_MCTS = min_MCTS < dur ? min_MCTS : dur;
            sum_MCTS += dur;
            ++nstep_MCTS;
        } else {
            max_MCGS = max_MCGS > dur ? max_MCGS : dur;
            min_MCGS = min_MCGS < dur ? min_MCGS : dur;
            sum_MCGS += dur; 
            ++nstep_MCGS;
        }
        ++nstep;
        printf("Step %d\n", nstep);
    }

    printf("MCTS max: %f, min: %f, avg: %f\n", max_MCTS, min_MCTS, sum_MCTS / nstep_MCTS);
    printf("MCGS max: %f, min: %f, avg: %f\n", max_MCGS, min_MCGS, sum_MCGS / nstep_MCGS);
    return 0;
}