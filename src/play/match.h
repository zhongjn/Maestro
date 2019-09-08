#include "round.h"
#include <functional>
#include <iostream>

namespace Maestro {
    using namespace std;

    struct MatchStat {
        int total = 0, current = 1;
        int p1_win = 0, p2_win = 0, draw = 0;
    };

    template<typename TGame>
    class Match {
        function<shared_ptr<IPlayer<TGame>>()> _p1_creator, _p2_creator;
        shared_ptr<IPlayer<TGame>> _p1, _p2;
        MatchStat _mstat;

    public:


        Match(int n_rounds, function<shared_ptr<IPlayer<TGame>>()> p1_creator, function<shared_ptr<IPlayer<TGame>>()> p2_creator)
            : _p1_creator(move(p1_creator)), _p2_creator(move(p2_creator)) {
            if (n_rounds % 2 == 1) ++n_rounds;
            _mstat.total = n_rounds;
        }

        bool step(bool rand_first_step = false, bool disp = true) {
            if (_mstat.current > _mstat.total) return false;
            _p1 = _p1_creator();
            _p2 = _p2_creator();

            auto pa = _p1, pb = _p2;
            bool swap_side = _mstat.current % 2 == 0;
            if (swap_side) swap(pa, pb);

            auto r = Round<TGame>(TGame(), move(pa), move(pb));
            if (rand_first_step) {
                r.rand_step();
            }
            r.step_to_end();
            if (disp) {
                puts(r.game().to_string().c_str());
            }
            Color winner = r.game().get_status().winner;
            int a_win = 0, b_win = 0, draw = 0;
            if (winner == Color::None) {
                ++draw;
            }
            else if (winner == Color::A) {
                ++a_win;
            }
            else if (winner == Color::B) {
                ++b_win;
            }

            ++_mstat.current;
            _mstat.draw += draw;
            _mstat.p1_win += !swap_side ? a_win : b_win;
            _mstat.p2_win += !swap_side ? b_win : a_win;
            if (disp) {
                printf("p1=%d, p2=%d, draw=%d\n", _mstat.p1_win, _mstat.p2_win, _mstat.draw);
            }
            return true;
        }

        void step_to_end(bool rand_first_step = false, bool disp = true) {
            while (_mstat.current <= _mstat.total) {
                printf("\rround (%d/%d)", _mstat.current, _mstat.total);
                step(rand_first_step, disp);
            }
            printf("\nmatch end! p1=%d, p2=%d, draw=%d\n", _mstat.p1_win, _mstat.p2_win, _mstat.draw);
        }

        const MatchStat& stat() {
            return _mstat;
        }

        const shared_ptr<IPlayer<TGame>>& player(int no) {
            if (no == 1) {
                return _p1;
            }
            if (no == 2) {
                return _p2;
            }
            else {
                throw logic_error("invalid player no");
            }
        }
    };
}