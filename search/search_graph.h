#pragma once
#include "../util/lazy.h"
#include "../util/expirable.h"
#include "search_base.h"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace Maestro {
    using namespace std;

    const float PUCT = 2;
    const float NOISE_EPSILON = 0.25;

    template<typename TGame>
    class MonteCarloGraphSearch : public IMonteCarloSearch<TGame> {
    public:
        struct GlobalStat {
            int sim_use_transposition = 0,
                sim_total = 0;
            map<int, int> children_evaluated_dist;
            float tt_load_factor = 0;
        } inline static global_stat = GlobalStat();

    private:

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
            Expirable<int> backup_visited_child_count;
            Expirable<int> backup_total_child_count;

            float convert_v(Color color, float v) const {
                return color == game.get_color() ? v : -v;
            }

            float convert_v(State* state) const {
                return convert_v(state->game.get_color(), state->v);
            }

            State(Transposition* tt, TGame game) : game(game) {
                child_actions = [this, tt]() {
                    vector<shared_ptr<Action>> actions;
                    assert(eval);
                    for (auto& mp : eval->p) {
                        actions.push_back(make_shared<Action>(tt, mp.move, weak_from_this(), mp.p));
                    }
                    return actions;
                };
            }
        };

        struct Action : public enable_shared_from_this<Action> {
            Move<TGame> move;
            int visit = 0;
            float p = 0;
            float p_noise_delta = 0;
            weak_ptr<State> parent_state;
            Lazy<shared_ptr<State>> child_state;

            Action(Transposition* tt, Move<TGame> move, weak_ptr<State> parent, float p) : move(move), parent_state(parent), p(p) {
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
        shared_ptr<IEvaluator<TGame>> _evaluator;
        int _timestamp = 0;
        bool _dirichlet_noise;

        vector<State*> _sim_stack;
        vector<State*> _backup_stack;

        float action_ucb(State* parent, Action* action) {
            static uniform_real_distribution<float> dist(0, 1E-3);
            float u = 0;
            if (action->child_state.initialized()) {
                u += parent->convert_v(action->child_state.value().get());
            }
            u += dist(_rnd_eng);
            u += PUCT * (action->p + action->p_noise_delta) * sqrtf(parent->ns) / (1 + action->visit);
            return u;
        }

        void backup_dv(State * origin) {

            int visit_expect = 0, visit_actual = 0;

            // 1. 计算子DAG各节点孩子数量
            _backup_stack.clear();
            _backup_stack.push_back(origin);
            while (!_backup_stack.empty()) {
                State* cur = _backup_stack.back();
                _backup_stack.pop_back();
                ++visit_expect;
                for (auto& wpa : cur->parent_actions) {
                    if (shared_ptr<Action> pa = wpa.lock(); pa) {
                        shared_ptr<State> ps = pa->parent_state.lock();
                        assert(ps);
                        int child_before = ps->backup_total_child_count(_timestamp);
                        if (child_before == 0) {
                            _backup_stack.push_back(ps.get());
                        }
                        ++ps->backup_total_child_count(_timestamp);
                    }
                }
            }

            // 2. 对子DAG按逆拓扑序遍历（即从最子节点开始），传播dv
            _backup_stack.clear();
            _backup_stack.push_back(origin);
            while (!_backup_stack.empty()) {
                State* cur = _backup_stack.back();
                _backup_stack.pop_back();
                ++visit_actual;

                float cur_v_before = cur->v;
                // 累加dv到v上
                cur->v += cur->dv(_timestamp);
                float cur_v_after = cur->v;

                for (auto& wpa : cur->parent_actions) {
                    shared_ptr<Action> pa = wpa.lock();
                    if (pa) {
                        shared_ptr<State> ps = pa->parent_state.lock();
                        assert(ps);
                        Color cur_color = cur->game.get_color();

                        // 从子状态dv计算出当前dv，累加
                        ps->dv(_timestamp) += (ps->convert_v(cur_color, cur_v_after) - ps->convert_v(cur_color, cur_v_before)) / ps->ns;

                        ps->backup_visited_child_count(_timestamp)++;
                        if (ps->backup_visited_child_count(_timestamp) == ps->backup_total_child_count(_timestamp)) {
                            _backup_stack.push_back(ps.get());
                        }
                    }
                }
            }

            assert(visit_expect == visit_actual);
        }

        void slow_dfs_traversal(function<void(State*)> fn) {
            vector<State*> stack;
            unordered_set<State*> visited;
            stack.push_back(_root.get());
            while (!stack.empty()) {
                State* current = stack.back();
                stack.pop_back();

                visited.insert(current);
                fn(current);

                if (current->child_actions.initialized()) {
                    for (auto& ac : current->child_actions.value()) {
                        if (ac->child_state.initialized()) {
                            State* ptr = ac->child_state.value().get();
                            // not visited
                            if (visited.find(ptr) == visited.end()) {
                                stack.push_back(ptr);
                            }
                        }
                    }
                }
            }
        }

        vector<float> rand_dirichlet(int n, float concentration)
        {
            std::vector<float> ret = std::vector<float>(n, 0);
            std::gamma_distribution<float> gamma(concentration, 1);
            float sum = 0;
            for (int i = 0; i < n; ++i) {
                ret[i] = gamma(_rnd_eng);
                sum += ret[i];
            }
            for (int i = 0; i < n; ++i) {
                ret[i] = ret[i] / sum;
            }
            return ret;
        }

        void apply_dirichlet_noise() {
            auto all_moves = _root->game.get_all_legal_moves();
            auto noise = rand_dirichlet(all_moves.size(), 0.03);

            int i = 0;
            for (auto& m : all_moves) {
                bool found = false;
                Action* action = nullptr;
                for (auto& ac : _root->child_actions.value()) {
                    if (ac->move == m) {
                        found = true;
                        action = ac.get();
                        break;
                    }
                }
                if (!found) {
                    auto ac = make_shared<Action>(&_transposition, m, weak_ptr(_root), 0);
                    action = ac.get();
                    _root->child_actions->push_back(ac);
                }
                action->p_noise_delta = -NOISE_EPSILON * action->p + NOISE_EPSILON * noise[i];
                ++i;
            }
        }

        void collect_children_evaluated_dist() {
            global_stat.children_evaluated_dist.clear();
            map<int, int> dist;
            int max_count = 0;
            slow_dfs_traversal([&dist, &max_count](State * state) {
                if (state->child_actions.initialized()) {
                    int count = 0;
                    for (auto& ac : state->child_actions.value()) {
                        if (ac->child_state.initialized()) {
                            if (ac->child_state.value()->evaluated) {
                                count++;
                            }
                        }
                    }
                    ++dist[count];
                    max_count = max(count, max_count);
                }
                else {
                    ++dist[0];
                }
            });
            global_stat.children_evaluated_dist = std::move(dist);
        }

    public:

        MonteCarloGraphSearch(shared_ptr<IEvaluator<TGame>> evaluator, TGame game, bool dirichlet_noise = false) : _evaluator(std::move(evaluator)), _dirichlet_noise(dirichlet_noise) {
            _root = make_shared<State>(&_transposition, game);
        }

        virtual void simulate(int k) {
            global_stat.sim_total = 0;
            global_stat.sim_use_transposition = 0;

            for (int i = 0; i < k; i++) {
                global_stat.sim_total++;
                _timestamp++;
                _sim_stack.clear();

                _root->ns++;
                State* current = _root.get();

                float backup_v = 0;
                bool use_transposition = false;

                while (true) {
                    _sim_stack.push_back(current);
                    //current->visit++;

                    auto& game = current->game;
                    Status stat = game.get_status();

                    if (stat.end) {
                        if (stat.winner == game.get_color()) {
                            backup_v = 1;
                        }
                        else if (stat.winner == Color::None) {
                            backup_v = 0;
                        }
                        else {
                            backup_v = -1;
                        }
                        break;
                    }
                    else if (!current->evaluated) {
                        current->eval = make_unique<Evaluation<TGame>>(_evaluator->evaluate(game));
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
                    Action* action = nullptr;
                    float max_ucb = -100000;
                    for (auto& ac : actions) {
                        float ucb = action_ucb(current, ac.get());
                        if (ucb > max_ucb) {
                            max_ucb = ucb;
                            action = ac.get();
                        }
                    }


                    State* next = action->child_state.value().get();
                    int ns_before = next->ns;
                    action->visit++;
                    int ns_after = next->ns = max(next->ns, action->visit);

                    if (ns_before == ns_after) {
                        global_stat.sim_use_transposition++;
                        use_transposition = true;
                    }

                    current = next;
                }

                State* leaf = _sim_stack.back();
                leaf->dv(_timestamp) += (backup_v - leaf->v) / leaf->ns;

                // 纠正路径上的v（由ns+1引起）
                for (int i = 0; i < _sim_stack.size() - 1; i++) {
                    State* cur = _sim_stack[i];
                    State* next = _sim_stack[i + 1];
                    cur->dv(_timestamp) += (cur->convert_v(next) - cur->v) / cur->ns;
                }

                backup_dv(leaf);
            }

            collect_children_evaluated_dist();
            global_stat.tt_load_factor = _transposition.load_factor();
        }

        virtual vector<MoveVisit> get_moves() const {
            vector<MoveVisit> mvs;
            for (auto& ap : _root->child_actions.value()) {
                mvs.push_back(MoveVisit{ ap->move, ap->visit });
            }
            return mvs;
        }

        virtual float get_value(Color color) const {
            return  (_root->game.get_color() == color ? 1 : -1) * _root->v;
        }

        virtual TGame get_game_snapshot() const {
            return _root->game;
        }

        virtual void move(Move<TGame> move) {
            TGame g = _root->game;
            g.move(move);
            _root = make_shared<State>(&_transposition, g);

            if (_dirichlet_noise) {
                apply_dirichlet_noise();
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