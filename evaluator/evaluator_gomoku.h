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
            int b, w;
            int max;
            float sum = 0;
            float v;
            judge(game.black, game.white, b, w);
            v = cur_color != Color::A ? b : w;
            v = v / 5;

            for (Move<Gomoku>& m : moves) {
                Gomoku g = game;
                g.move(m);
                judge(g.black, g.white, b, w);
                max = cur_color == Color::A ? b : w;
                sum += max;
                p.push_back(MovePrior<Gomoku>{m, float(max)});

            }

            for (MovePrior<Gomoku>& mp : p) {
                mp.p = mp.p / sum;
            }
            return Evaluation<Gomoku>{move(p), v};
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
                    for (int k = 0; k < 6; k++) {
                        connect &= hb.get(r + k * dr, c + k * dc);
                        if (!connect) {
                            max = max < k ? k : max;
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
