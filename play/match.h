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
        function<unique_ptr<IPlayer<TGame>>()> _p1_creator, _p2_creator;
        MatchStat _mstat;

    public:


        Match(int n_rounds, function<unique_ptr<IPlayer<TGame>>()> p1_creator, function<unique_ptr<IPlayer<TGame>>()> p2_creator)
            : _p1_creator(move(p1_creator)), _p2_creator(move(p2_creator)) {
            if (n_rounds % 2 == 1) ++n_rounds;
            _mstat.total = n_rounds;
        }

        bool step() {
            if (_mstat.current > _mstat.total) return false;

            auto pa = _p1_creator(), pb = _p2_creator();
            bool swap_side = _mstat.current % 2 == 0;
            if (swap_side) swap(pa, pb);

            auto r = Round<TGame>(TGame(), move(pa), move(pb));
            r.step_to_end();
            puts(r.game().to_string().c_str());
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
            printf("p1=%d, p2=%d, draw=%d\n", _mstat.p1_win, _mstat.p2_win, _mstat.draw);
            return true;
        }

        void step_to_end() {
            while (step()) {}
            printf("match end! p1=%d, p2=%d, draw=%d\n", _mstat.p1_win, _mstat.p2_win, _mstat.draw);
        }

        const MatchStat& stat() {
            return _mstat;
        }
    };
}