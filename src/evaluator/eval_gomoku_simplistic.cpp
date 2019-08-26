#include <maestro/evaluator/eval_gomoku_simplistic.h>

using namespace Maestro;

Evaluation<Gomoku> Maestro::SimplisticGomokuEvaluator::evaluate(const Gomoku& game) {
    minstd_rand rnd_eng;
    rnd_eng.seed(game.get_hash());
    uniform_real_distribution<float> dist(0, 1E-3);

    std::vector<MovePrior<Gomoku>> p;
    p.reserve(10);

    float sum = 0;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            Move<Gomoku> m{ r, c };
            if (game.is_legal_move_unchecked(m)) {
                bool found = false;
                for (int dr = -1; dr <= 1; dr++) {
                    for (int dc = -1; dc <= 1; dc++) {
                        if (game.black.safe_get(r + dr, c + dc) || game.white.safe_get(r + dr, c + dc)) {
                            found = true;
                            goto break_inner;
                        }
                    }
                }
            break_inner:
                if (found) {
                    float prior = found ? 1 : 0.1;
                    prior = 1;
                    prior += dist(rnd_eng);
                    sum += prior;
                    p.push_back(MovePrior<Gomoku>{m, prior});
                }
            }
        }
    }

    if (p.size() == 0) {
        sum += 1;
        p.push_back(MovePrior<Gomoku>{Move<Gomoku>{7, 7}, 1});
    }

    for (MovePrior<Gomoku>& mp : p) {
        mp.p = mp.p / sum;
    }

    return Evaluation<Gomoku>{move(p), 0};
}

void Maestro::SimplisticGomokuEvaluator::set_check_interval(int d, int& start, int& end) {
    if (d == 0) {
        start = 0;
        end = BOARD_SIZE - 1;
    }
    else if (d == -1) {
        start = 4;
        end = BOARD_SIZE - 1;
    }
    else if (d == 1) {
        start = 0;
        end = BOARD_SIZE - 5;
    }
    else {
        assert(0);
    }
}

int Maestro::SimplisticGomokuEvaluator::check_dir(const Gomoku::HalfBoard& hb, int dr, int dc) {
    int r_start, r_end, c_start, c_end;
    set_check_interval(dr, r_start, r_end);
    set_check_interval(dc, c_start, c_end);
    int max = 0;

    for (int r = r_start; r <= r_end; r++) {
        for (int c = c_start; c <= c_end; c++) {
            bool connect = true;
            for (int k = 0; k < 5; k++) {
                connect &= hb.get(r + k * dr, c + k * dc);
                if (!connect) {
                    max = k > max ? k : max;
                    break;
                }
            }
        }
    }
    return max;
}

void Maestro::SimplisticGomokuEvaluator::judge(const Gomoku::HalfBoard& b, const Gomoku::HalfBoard& w, int& max_b, int& max_w) {
    max_b = 0;
    max_w = 0;
    int max = check_dir(b, 1, 0);
    max_b = max_b < max ? max : max_b;
    max = check_dir(b, 0, 1);
    max_b = max_b < max ? max : max_b;
    max = check_dir(b, 1, 1);
    max_b = max_b < max ? max : max_b;
    max = check_dir(b, 1, -1);
    max_b = max_b < max ? max : max_b;
    max = check_dir(w, 1, 0);
    max_w = max_w < max ? max : max_w;
    max = check_dir(w, 0, 1);
    max_w = max_w < max ? max : max_w;
    max = check_dir(w, 1, 1);
    max_w = max_w < max ? max : max_w;
    max = check_dir(w, 1, -1);
    max_w = max_w < max ? max : max_w;
}
