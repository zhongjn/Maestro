#pragma once
#include "search_base.h"
#include <unordered_map>
#include "util/lazy.h"

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class MonteCarloGraphSearch : public IMonteCarloSearch<TGame> {

        // struct definations
        struct GameHash {
            size_t operator()(const TGame& game) {
                return game.get_hash();
            }
        };

        class Action;

        class State {
            bool _expanded = false;
        public:
            TGame game;
            vector<weak_ptr<Action>> parent_actions;
            Lazy<vector<shared_ptr<Action>>> child_actions;
            int ns = 0;
            float sum_nv_v = 0;
            float sum_nv = 0;
        };

        class Action {
            Move<TGame> move;
        };

        // members
        unordered_map<TGame, shared_ptr<State>> _transposition;
        unique_ptr<IEvaluator<TGame>> _evaluator;

    public:

        MonteCarloGraphSearch(unique_ptr<IEvaluator<TGame>> evaluator) : _evaluator(move(evaluator)) {

        }

        // TODO @zjn
        virtual void simulate(int k) {

        }

        virtual vector<MoveVisit> get_moves() const {

        }

        virtual float get_value() const {

        }

        virtual unique_ptr<TGame> get_game_snapshot() const {
        }

        virtual void move(Move<TGame> move) {

        }
    };
}