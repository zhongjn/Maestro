#pragma once
#include <vector>
#include <memory>
#include "abstract_game.h"
#include <cassert>

namespace Maestro {
    using namespace std;

    template<typename TObsv, typename TMov>
    class IMonteCarloSearch {
    public:
        struct MoveVisit {
            TMov move;
            int visit_count;
        };
        virtual void simulate(int k) = 0;
        virtual vector<MoveVisit> get_moves() const = 0;
        virtual float get_value() const = 0;
        virtual unique_ptr<IGame<TObsv, TMov>> get_game_snapshot() const = 0;
        virtual void move(TMov move) = 0;
        void move_best() {
            auto moves = get_moves();
            int max_visit = -1;
            TMov move;
            assert(moves.size() > 0);
            for (auto& m : moves) {
                if (m.visit_count > max_visit) {
                    move = m.move;
                }
            }
        }
    };
}