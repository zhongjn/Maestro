#pragma once
#include "util/lazy.h"
#include "util/expirable.h"
#include "search_base.h"
#include <unordered_map>
#include <stack>
#include <algorithm>

namespace Maestro {
    using namespace std;

    template<typename TGame>
    class MonteCarloGraphSearch : public IMonteCarloSearch<TGame> {

        // struct definations
        struct GameHash {
            size_t operator()(const TGame& game) const {
                return game.get_hash();
            }
        };

        struct Action;
        struct State;

        using Transposition = unordered_map<TGame, shared_ptr<State>, GameHash>;

        struct State : public enable_shared_from_this<State> {
            bool evaluated = false;
            unique_ptr<Evaluation<TGame>> eval;
            TGame game;
            int ns = 0;
            float v = 0;
            Expirable<float> dv;
            //float visit = 0;
            vector<weak_ptr<Action>> parent_actions;
            Lazy<vector<shared_ptr<Action>>> child_actions;
            Expirable<int> child_visited;

            float convert_v(Player player, float v) const {
                // TODO: swap
                return player == game.get_player() ? v : -v;
            }

            State(Transposition* tt, TGame game) : game(game) {
                child_actions = [this, tt]() {
                    vector<shared_ptr<Action>> actions;
                    assert(eval);
                    for (auto& mp : eval->p) {
                        actions.push_back(make_shared<Action>(tt, mp.move, weak_from_this()));
                    }
                    return actions;
                };
            }
        };

        struct Action : public enable_shared_from_this<Action> {
            Move<TGame> move;
            int visit = 0;
            weak_ptr<State> parent_state;
            Lazy<shared_ptr<State>> child_state;
            Action(Transposition* tt, Move<TGame> move, weak_ptr<State> parent) : move(move), parent_state(parent) {
                child_state = [this, tt]() {
                    shared_ptr<State> parent = parent_state.lock();
                    assert(parent);

                    TGame game = parent->game; // copy the game
                    game.move(this->move);

                    auto& entry = tt->find(game);
                    shared_ptr<State> child;
                    if (entry != tt->end()) {
                        child = entry->second;
                    }
                    else {
                        child = make_shared<State>(tt, game);
                        tt->insert(pair<TGame, shared_ptr<State>>(game, child));
                    }
                    child->parent_actions.push_back(weak_from_this());
                    return child;
                };
            }
        };

        shared_ptr<State> _root;
        // members
        Transposition _transposition;
        unique_ptr<IEvaluator<TGame>> _evaluator;
        int _timestamp = 0;

        vector<State*> _sim_stack;
        vector<State*> _backup_stack;

        void backup_dv(State* origin) {
            _backup_stack.clear();
            _backup_stack.push_back(origin);
            while (!_backup_stack.empty()) {
                State* cur = _backup_stack.back();
                _backup_stack.pop_back();

                float cur_v_before = cur->v;
                // �ۼ�dv��v��
                cur->v += cur->dv(_timestamp);
                float cur_v_after = cur->v;

                for (auto& wpa : cur->parent_actions) {
                    shared_ptr<Action> pa = wpa.lock();
                    if (pa) {
                        shared_ptr<State> ps = pa->parent_state.lock();
                        assert(ps);
                        Player cur_player = cur->game.get_player();

                        // ����״̬dv�������ǰdv���ۼ�
                        ps->dv(_timestamp) += (ps->convert_v(cur_player, cur_v_after) - ps->convert_v(cur_player, cur_v_before)) / ps->ns;

                        ps->child_visited(_timestamp)++;
                        if (ps->child_visited(_timestamp) == ps->child_actions->size()) {
                            _backup_stack.push_back(ps.get());
                        }
                    }
                }
            }
        }

    public:

        MonteCarloGraphSearch(unique_ptr<IEvaluator<TGame>> evaluator, TGame game) : _evaluator(std::move(evaluator)) {
            _root = make_shared<State>(&_transposition, game);
        }

        // TODO @zjn
        virtual void simulate(int k) {
            for (int i = 0; i < k; i++) {
                _timestamp++;
                _sim_stack.clear();

                shared_ptr<State> current = _root;
                float backup_v = 0;
                bool use_transposition = false;

                while (true) {
                    _sim_stack.push_back(current.get());
                    //current->visit++;

                    auto& game = current->game;
                    Status stat = game.get_status();

                    if (stat.end) {
                        backup_v = stat.winner != Player::None ? 1 : 0;
                        break;
                    }
                    else if (!current->evaluated) {

                        current->eval = make_unique<Evaluation<TGame>>(_evaluator->evaluate(game.get_obsv(game.get_player())));
                        current->evaluated = true;
                        float value = current->eval->v;
                        backup_v = value;
                        break;
                    }
                    else if (use_transposition) {
                        backup_v = current->v;
                        break;
                    }

                    vector<shared_ptr<Action>>& actions = current->child_actions.value();
                    // TODO: select action
                    shared_ptr<Action> action;

                    shared_ptr<State> next = action->child_state.value();
                    int ns_before = next->ns;
                    action->visit++;
                    int ns_after = next->ns = max(next->ns, action->visit);

                    if (ns_before == ns_after) {
                        use_transposition = true;
                    }

                    current = next;
                }

                State* leaf = _sim_stack.back();
                leaf->dv(_timestamp) += (backup_v - leaf->v) / (leaf->ns + 1);

                // ����·���ϵ�v����ns+1����
                for (int i = 0; i < _sim_stack.size() - 1; i++) {
                    State* cur = _sim_stack[i];
                    State* next = _sim_stack[i + 1];
                    cur->dv(_timestamp) += (cur->convert_v(next->game.get_player(), next->v) - cur->v) / (cur->ns + 1);
                }

                backup_dv(leaf);
            }
        }

        virtual vector<MoveVisit> get_moves() const {
            vector<MoveVisit> mvs;
            for (auto& ap : _root->child_actions.value()) {
                mvs.push_back(MoveVisit{ ap->move, ap->visit });
            }
            return mvs;
        }

        virtual float get_value(Player player) const {
            return  (_root->game.get_player() == player ? 1 : -1) * _root->v;
        }

        virtual TGame get_game_snapshot() const {
            return _root->game;
        }

        virtual void move(Move<TGame> move) {
            for (auto& ap : _root->child_actions.value()) {
                if (ap->move == move) {
                    _root = ap->child_state.value();
                }
            }

            for (auto it = begin(_transposition); it != end(_transposition);)
            {
                if (!_root->game.could_transfer_to(it->first))
                {
                    it = _transposition.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    };
}