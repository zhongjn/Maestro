#include "../search/search_base.h"
#include "../game/game_gomoku.h"
#include <iostream>

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class IPlayer {
    public:
        virtual Move<TGame> get_move(const TGame& game) = 0;
        virtual void move(Move<TGame> m) = 0;
    };

    template<typename TGame>
    class MonteCarloAIPlayer final : public IPlayer<TGame> {
        unique_ptr<IMonteCarloSearch<TGame>> _search;
        int _n_sim;
    public:
        MonteCarloAIPlayer(unique_ptr<IMonteCarloSearch<TGame>> search, int n_sim) : _search(std::move(search)), _n_sim(n_sim) {

        }

        virtual Move<TGame> get_move(const TGame& game) {
            assert(game == _search->get_game_snapshot());
            _search->simulate(_n_sim);
            auto m = _search->pick_move(0.0f);
            return m;
        }

        virtual void move(Move<TGame> m) {
            _search->move(m);
        }
    };

    template<>
    class GomokuConsolePlayer final : public IPlayer<Gomoku> {
    public:
        virtual Move<Gomoku> get_move(const Gomoku& game) {
            cout << game.to_string() << endl;
            while (true) {
                cout << "your move?" << endl;
                char move_cc;
                int move_r;
                cin >> move_cc >> move_r;
                int move_c = move_cc - 'A';
                assert(move_c >= 0 && move_c < BOARD_SIZE);
                Move<Gomoku> m{ move_r, move_c };
                if (game.is_legal_move(m)) {
                    return m;
                }
                else {
                    cout << "illegal move!" << endl;
                }
            }
        }

        virtual void move(Move<Gomoku> m) {

        }
    };
}