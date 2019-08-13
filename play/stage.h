#include "player.h"

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class Stage {
        TGame _game;
        unique_ptr<IPlayer<TGame>> _pa, _pb;
    public:
        Stage(unique_ptr<IPlayer<TGame>> pa, unique_ptr<IPlayer<TGame>> pb) : _pa(std::move(pa)), _pb(std::move(pb)) {

        }
        const TGame& game() { return _game; }
        void step() {
            if (_game.get_status().end) return;
            Color c = _game.get_color();
            Move<Gomoku> m;
            if (c == Color::A) {
                m = _pa->get_move(_game);
            }
            else if (c == Color::B) {
                m = _pb->get_move(_game);
            }
            else {
                assert(false);
            }
            _game.move(m);
            _pa->move(m);
            _pb->move(m);
        }
    };
}