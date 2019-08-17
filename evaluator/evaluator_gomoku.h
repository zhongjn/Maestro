#pragma once
#include "../game/game_base.h"
#include "../game/game_gomoku.h"

namespace Maestro {
    class SimplisticGomokuEvaluator final : public IEvaluator<Gomoku> {
    public:
        Evaluation<Gomoku> evaluate(const Gomoku& game) {
            Color cur_color = game.get_color();
            std::vector<MovePrior<Gomoku>> p;
            std::vector<Move<Gomoku>> moves = game.get_all_legal_moves();

            float sum = 0;
            for (Move<Gomoku>& m : moves) {
                bool found = false;
                for (int dr = -1; dr <= 1; dr++) {
                    for (int dc = -1; dc <= 1; dc++) {
                        int r = m.row + dr, c = m.col + dc;
                        if (game.black.safe_get(r, c) || game.white.safe_get(r, c)) {
                            found = true;
                            goto break_inner;
                        }
                    }
                }
            break_inner:
                if (found) {
                    float prior = found ? 1 : 0.1;
                    prior = 1;
                    sum += prior;
                    p.push_back(MovePrior<Gomoku>{m, prior});
                }
            }

            if (p.size() == 0) {
                sum += 1;
                p.push_back(MovePrior<Gomoku>{Move<Gomoku>{5, 5}, 1});
            }

            for (MovePrior<Gomoku>& mp : p) {
                mp.p = mp.p / sum;
            }

            return Evaluation<Gomoku>{move(p), 0};
        }
    private:
        void set_check_interval(int d, int& start, int& end) {
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
        int check_dir(const Gomoku::HalfBoard& hb, int dr, int dc) {
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

        void judge(const Gomoku::HalfBoard& b, const Gomoku::HalfBoard& w, int& max_b, int& max_w) {
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
    };
}
