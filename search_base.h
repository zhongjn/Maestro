#pragma once
#include <vector>
#include <memory>
#include "game_base.h"
#include <cassert>

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class IMonteCarloSearch {
    public:

        struct MoveVisit {
            Move<TGame> move;
            int visit_count;
        };
        virtual void simulate(int k) = 0;
        virtual vector<MoveVisit> get_moves() const = 0;
        virtual float get_value() const = 0;
        virtual unique_ptr<TGame> get_game_snapshot() const = 0;
        virtual void move(Move<TGame> move) = 0;
        void move_best() {
            auto moves = get_moves();
            int max_visit = -1;
            Move<TGame> max_move;
            assert(moves.size() > 0);
            for (auto& m : moves) {
                if (m.visit_count > max_visit) {
                    max_move = m.move;
                }
            }
        }
    };
}