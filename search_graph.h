#pragma once
#include "search_base.h"
#include <unordered_map>

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class MonteCarloGraphSearch : public IMonteCarloSearch<TGame> {
        // type aliases

        // struct definations
        struct GameHash {
            size_t operator()(const TGame& game) {
                return game.get_hash();
            }
        };

        struct State {
            TGame game;

        };

        struct Action {

        };

        // members
        unordered_map<TGame, shared_ptr<State>> _transposition;


    public:

        // TODO @zjn
        virtual void simulate(int k) {
            shared_ptr<int> p;
            TGame t;
        }
        virtual vector<MoveVisit> get_moves() const {

        }
        virtual float get_value() const = 0;
        virtual unique_ptr<TGame> get_game_snapshot() const = 0;
        virtual void move(Move<TGame> move) = 0;
    };
}