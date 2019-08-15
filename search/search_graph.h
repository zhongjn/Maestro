#pragma once
#include "../util/lazy.h"
#include "../util/expirable.h"
#include "search_base.h"
#include <unordered_map>
#include <stack>
#include <algorithm>

namespace Maestro {
    using namespace std;

    const float PUCT = 2;

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

        int _sim_total = 0;
        int _sim_saved = 0;

        float action_ucb(State* parent, Action* action) {
            static uniform_real_distribution<float> dist(0, 1E-3);
            float u = 0;
            if (action->child_state.initialized()) {
                u += parent->convert_v(action->child_state.value().get());
            }
            u += dist(_rnd_eng);
            u += PUCT * action->p * sqrtf(parent->ns) / (1 + action->visit);
            return u;
        }

        void backup_dv(State * origin) {
            // 1. ������DAG���ڵ㺢������
            _backup_stack.clear();
            _backup_stack.push_back(origin);
            while (!_backup_stack.empty()) {
                State* cur = _backup_stack.back();
                _backup_stack.pop_back();
                for (auto& wpa : cur->parent_actions) {
                    if (shared_ptr<Action> pa = wpa.lock(); pa) {
                        shared_ptr<State> ps = pa->parent_state.lock();
                        assert(ps);
                        ps->backup_total_child_count(_timestamp)++;
                        _backup_stack.push_back(ps.get());
                    }
                }
            }

            // 2. ����DAG����������������������ӽڵ㿪ʼ��������dv
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
                        Color cur_color = cur->game.get_color();

                        // ����״̬dv�������ǰdv���ۼ�
                        ps->dv(_timestamp) += (ps->convert_v(cur_color, cur_v_after) - ps->convert_v(cur_color, cur_v_before)) / ps->ns;

                        ps->backup_visited_child_count(_timestamp)++;
                        if (ps->backup_visited_child_count(_timestamp) == ps->backup_total_child_count(_timestamp)) {
                            _backup_stack.push_back(ps.get());
                        }
                    }
                }
            }
        }

        void apply_dirichlet_noise() {
            // TODO
        }

    public:

        MonteCarloGraphSearch(shared_ptr<IEvaluator<TGame>> evaluator, TGame game, bool dirichlet_noise = false) : _evaluator(std::move(evaluator)), _dirichlet_noise(dirichlet_noise) {
            _root = make_shared<State>(&_transposition, game);
        }

        float save_ratio() const {
            return float(_sim_saved) / _sim_total;
        }

        virtual void simulate(int k) {
            for (int i = 0; i < k; i++) {
                _sim_total++;
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
                        _sim_saved++;
                        use_transposition = true;
                    }

                    current = next;
                }

                State* leaf = _sim_stack.back();
                leaf->dv(_timestamp) += (backup_v - leaf->v) / leaf->ns;

                // ����·���ϵ�v����ns+1����
                for (int i = 0; i < _sim_stack.size() - 1; i++) {
                    State* cur = _sim_stack[i];
                    State* next = _sim_stack[i + 1];
                    cur->dv(_timestamp) += (cur->convert_v(next) - cur->v) / cur->ns;
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