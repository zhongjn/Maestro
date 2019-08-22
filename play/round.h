#include "player.h"

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class Round {
        TGame _game;
        shared_ptr<IPlayer<TGame>> _pa, _pb;
    public:
        Round(TGame game, shared_ptr<IPlayer<TGame>> pa, shared_ptr<IPlayer<TGame>> pb) : _game(game), _pa(std::move(pa)), _pb(std::move(pb)) {

        }

        const TGame& game() { return _game; }

        bool step() {
            if (_game.get_status().end) {
                return false;
            }
            Color c = _game.get_color();
            Move<Gomoku> m;
            if (c == Color::A) {
                m = _pa->get_move(_game);
                _pb->get_move(_game);
            }
            else if (c == Color::B) {
                m = _pb->get_move(_game);
                _pa->get_move(_game);
            }
            else {
                throw runtime_error("unexpected color none");
            }
            _game.move(m);
            _pa->move(m);
            _pb->move(m);
            return true;
        }

        void step_to_end() {
            while (step()) {}
        }
    };
}