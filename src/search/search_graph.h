#pragma once
#include "../util/lazy.h"
#include "../util/expirable.h"
#include "search_base.h"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <functional>

namespace Maestro {
    using namespace std;

    const float NOISE_EPSILON = 0.25;

    template<typename TGame>
    class MonteCarloGraphSearch : public IMonteCarloSearch<TGame> {
    public:
        struct GlobalStat {
            int sim_use_transposition = 0,
                sim_game_end = 0,
                sim_total = 0;
            int visit_evaluating = 0;
            int node_evaluated_total = 0, node_evaluated_used = 0;
            int eval_batch_count = 0;
            float tt_load_factor = 0;
            float v_a = 0;
            void print() const {
                printf("game: v_a=%f\n", v_a);
                printf("sims: total=%d, game_end=%d, transposed=%d\n", sim_total, sim_game_end, sim_use_transposition);
                printf("eval: total=%d, used=%d, batch=%d\n", node_evaluated_total, node_evaluated_used, eval_batch_count);
                printf("misc: tt_load_factor=%f, visit_evaluating=%d\n", tt_load_factor, visit_evaluating);
            }
        } global_stat = GlobalStat();

        struct Config {
            bool same_response = true; // 去除对局随机性
            bool enable_dag = true;
            bool enable_speculative_evaluation = true;
            bool dirichlet_noise = false;
            int leaf_batch_count = 8;
            float puct = 2;
            float virtual_loss = 1;
        };

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
        using EvaluationResult = unordered_map<TGame, Evaluation<TGame>, GameHash>;

        struct State : public enable_shared_from_this<State> {
            bool stop_selection = false;
            bool evaluated = false;
            unique_ptr<Evaluation<TGame>> eval;
            TGame game;
            int ns = 0;
            float v = 0;
            Expirable<float> dv;
            Expirable<int> virtual_loss_cnt;
            bool noise_generated = false;
            bool evaluating = false;

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
        };

        struct Action : public enable_shared_from_this<Action> {
            Move<TGame> move;
            int visit = 0;
            float p = 0;
            float p_noise_delta = 0;
            weak_ptr<State> parent_state;
            Lazy<shared_ptr<State>> child_state;
        };

        Config _config;

        // members
        shared_ptr<State> _root;
        Transposition _transposition;
        EvaluationResult _eval_result;
        shared_ptr<IEvaluator<TGame>> _evaluator;

        enum class Timeline : int {
            origin = 0,
            backup_eval = 1,
            nop = 10
        };

        class TimelineManager {
            int _timestamp = 0;
        public:
            void next_epoch() { _timestamp += int(Timeline::nop); }
            int time() { return _timestamp; }
            int time(Timeline tl) { return _timestamp + int(tl); }
        } _timeline;

        vector<State*> _sim_stack;
        vector<State*> _backup_stack;

        shared_ptr<State> create_state(TGame game, bool root = false) {
            shared_ptr<State> ss;
            if (_config.enable_dag || root) {
                const auto& entry = _transposition.find(game);
                if (entry != _transposition.end()) {
                    ss = entry->second;
                }
            }

            if (!ss) {
                ss = make_shared<State>();
                ss->game = game;
                _transposition.insert(pair<TGame, shared_ptr<State>>(game, ss));
                State* s = ss.get();
                ss->child_actions = [this, s]() {
                    vector<shared_ptr<Action>> actions;
                    assert(s->eval);
                    for (auto& mp : s->eval->p) {
                        actions.push_back(create_action(mp.move, s->weak_from_this(), mp.p));
                    }
                    s->eval = nullptr;
                    return actions;
                };
            }

            return ss;
        }

        shared_ptr<Action> create_action(Move<TGame> m, weak_ptr<State> parent, float p) {
            auto acs = make_shared<Action>();
            Action* ac = acs.get();
            ac->move = m;
            ac->parent_state = parent;
            ac->p = p;
            ac->child_state = [this, ac]() {
                shared_ptr<State> parent = ac->parent_state.lock();
                assert(parent);

                TGame game = parent->game; // copy the game
                game.move(ac->move);

                shared_ptr<State> child = create_state(game);
                child->parent_actions.push_back(ac->weak_from_this());
                return child;
            };

            return acs;
        }

        float action_ucb(State* parent, Action* action, int ts_virtual_loss);

        void backup_dv(const vector<State*>& origins, Timeline tl = Timeline::origin);

        void slow_dfs_traversal(function<void(State*)> fn);

        vector<float> rand_dirichlet(int n, float concentration);

        void generate_root_dirichlet_noise();

    public:

        MonteCarloGraphSearch(
            shared_ptr<IEvaluator<TGame>> evaluator,
            TGame game,
            Config config = Config()) :
            _config(config),
            _evaluator(std::move(evaluator))
        {
            _root = create_state(game, true);

            if (!_config.same_response) {
                static int seed;
                seed += int(time(0));
                this->_rnd_eng.seed(seed);
            }
            else {
                this->_rnd_eng.seed(10);
            }
        }

        virtual void simulate(int k) override;

        virtual vector<MoveVisit<TGame>> get_moves() const override {
            vector<MoveVisit<TGame>> mvs;
            for (auto& ap : _root->child_actions.value()) {
                mvs.push_back(MoveVisit<TGame>{ ap->move, ap->visit });
            }
            return mvs;
        }

        virtual float get_value(Color color) const override {
            return  (_root->game.get_color() == color ? 1 : -1) * _root->v;
        }

        virtual TGame get_game_snapshot() const override {
            return _root->game;
        }

        virtual void move(Move<TGame> move) override;

        virtual void print_stat() const override {
            global_stat.print();
        }
    };

    template<typename TGame>
    inline float MonteCarloGraphSearch<TGame>::action_ucb(State* parent, Action* action, int ts_virtual_loss) {

        float u = 0;
        if (action->child_state.initialized()) {
            State* child = action->child_state.value().get();
            if (action->visit > 0) u += parent->convert_v(child);
            u -= child->virtual_loss_cnt(ts_virtual_loss) * _config.virtual_loss / max(1, child->ns);
        }
        u += _config.puct * (action->p + action->p_noise_delta) * sqrtf(parent->ns) / (1 + action->visit);
        return u;
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::backup_dv(const vector<State*>& origins, Timeline tl) {

        int visit_expect = 0, visit_actual = 0;
        int ts = _timeline.time(tl);

        // 1. 计算子DAG各节点孩子数量
        _backup_stack.clear();
        for (auto o : origins) {
            _backup_stack.push_back(o);
        }
        while (!_backup_stack.empty()) {
            State* cur = _backup_stack.back();
            _backup_stack.pop_back();
            ++visit_expect;
            for (auto& wpa : cur->parent_actions) {
                if (shared_ptr<Action> pa = wpa.lock(); pa) {
                    shared_ptr<State> ps = pa->parent_state.lock();
                    assert(ps);
                    int child_before = ps->backup_total_child_count(ts);
                    if (child_before == 0) {
                        _backup_stack.push_back(ps.get());
                    }
                    ++ps->backup_total_child_count(ts);
                }
            }
        }

        // 2. 对子DAG按逆拓扑序遍历（即从最子节点开始），传播dv
        _backup_stack.clear();
        for (auto o : origins) {
            _backup_stack.push_back(o);
        }
        while (!_backup_stack.empty()) {
            State* cur = _backup_stack.back();
            _backup_stack.pop_back();
            ++visit_actual;

            float cur_v_before = cur->v;
            // 累加dv到v上
            cur->v += cur->dv(ts);
            float cur_v_after = cur->v;

            for (auto& wpa : cur->parent_actions) {
                shared_ptr<Action> pa = wpa.lock();
                if (pa) {
                    shared_ptr<State> ps = pa->parent_state.lock();
                    assert(ps);
                    Color cur_color = cur->game.get_color();

                    // 从子状态dv计算出当前dv，累加
                    ps->dv(ts) += (ps->convert_v(cur_color, cur_v_after) - ps->convert_v(cur_color, cur_v_before)) * pa->visit / ps->ns;

                    ps->backup_visited_child_count(ts)++;
                    if (ps->backup_visited_child_count(ts) == ps->backup_total_child_count(ts)) {
                        _backup_stack.push_back(ps.get());
                    }
                }
            }
        }

        assert(visit_expect == visit_actual);
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::slow_dfs_traversal(function<void(State*)> fn) {
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

    template<typename TGame>
    inline vector<float> MonteCarloGraphSearch<TGame>::rand_dirichlet(int n, float concentration)
    {
        std::vector<float> ret = std::vector<float>(n, 0);
        std::gamma_distribution<float> gamma(concentration, 1);
        float sum = 0;
        for (int i = 0; i < n; ++i) {
            ret[i] = gamma(this->_rnd_eng);
            sum += ret[i];
        }
        for (int i = 0; i < n; ++i) {
            ret[i] = ret[i] / sum;
        }
        return ret;
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::generate_root_dirichlet_noise() {
        if (_root->noise_generated) return;
        _root->noise_generated = true;
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
                auto ac = create_action(m, weak_ptr(_root), 0);
                action = ac.get();
                _root->child_actions->push_back(ac);
            }
            action->p_noise_delta = -NOISE_EPSILON * action->p + NOISE_EPSILON * noise[i];
            ++i;
        }
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::simulate(int k) {
        // global_stat = GlobalStat();

        int cur_leaf_batch_count = 0;
        vector<State*> eval_batch;

        int ts_last_batch = 0;

        for (int i_sim = 1; i_sim <= k; i_sim++) {
            //printf("sim #%d begin\n", i_sim);
            bool evaluating_node_visited = false;

            global_stat.sim_total++;
            _timeline.next_epoch();
            _sim_stack.clear();

            _root->ns++;
            // 根节点需要特殊处理
            if (!_root->evaluated) {
                _root->eval = make_unique<Evaluation<TGame>>(_evaluator->evaluate(_root->game, _rnd_eng));
                _root->evaluated = true;
                _root->v = _root->eval->v;
                continue;
            }

            if (_config.dirichlet_noise) {
                generate_root_dirichlet_noise();
            }

            State* current = _root.get();

            float backup_v = 0;
            bool use_transposition = false;

            while (true) {
                _sim_stack.push_back(current);
                ++current->virtual_loss_cnt(ts_last_batch);

                auto& game = current->game;
                Status stat = game.get_status();

                if (stat.end) {
                    global_stat.sim_game_end++;
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

                if (use_transposition) {
                    global_stat.sim_use_transposition++;
                    backup_v = current->v;
                    break;
                }

                if (current->stop_selection) {
                    backup_v = current->v;
                    break;
                }

                if (!current->evaluated) {
                    if (current->evaluating) {
                        global_stat.visit_evaluating++;
                        evaluating_node_visited = true;
                        backup_v = 0;
                        break;
                    }
                    else {
                        throw runtime_error("this should never happen");
                    }
                }

                vector<shared_ptr<Action>>& actions = current->child_actions.value();

                vector<float> ac_p_temp;
                ac_p_temp.reserve(actions.size());
                Action* action = nullptr;
                int max_idx = -1;
                float max_ucb = -100000;
                int idx = 0;
                for (auto& ac : actions) {
                    float ucb = action_ucb(current, ac.get(), ts_last_batch);
                    ac_p_temp.push_back(ac->p + ac->p_noise_delta);
                    if (ucb >= max_ucb) {
                        max_ucb = ucb;
                        max_idx = idx;
                        action = ac.get();
                    }
                    idx++;
                }

                ac_p_temp[max_idx] += 1000; // 重要！保证选中的action被排序到最前

                if (!action->child_state.initialized() && action->child_state.value().get()->ns == 0) {
                    // 生成前k个未初始化的子节点
                    // 加入估值队列，设置其为evaluating
                    vector<int> ac_no;
                    ac_no.reserve(actions.size());
                    for (int i = 0; i < actions.size(); i++) {
                        ac_no.push_back(i);
                    }

                    // TODO: 可以用堆排优化
                    stable_sort(ac_no.begin(), ac_no.end(), [&ac_p_temp](int no1, int no2) { return ac_p_temp[no1] > ac_p_temp[no2]; });

                    int target_cnt = 0;
                    if (_config.enable_speculative_evaluation) {
                        int t = 0;
                        for (int no : ac_no) {
                            if (actions[no]->child_state.initialized()) {
                                State* cs = actions[no]->child_state.value().get();
                                if (cs->ns > 0) {
                                    t++;
                                }
                            }
                        }

                        target_cnt = clamp(int(t * 1), 0, 20000); // 启发函数
                    }

                    // tricky
                    // 即使target_cnt=0（不启用投机），下面这个循环也会被至少执行一次
                    // 这样就可以确保不启用投机时的正确性
                    int cur_cnt = 0;
                    int dd = 0;
                    for (int no : ac_no) {
                        State* cs = actions[no]->child_state.value().get();
                        if (cs->ns == 0) {
                            if (!cs->game.get_status().end && !cs->evaluating && !cs->evaluated) {
                                cur_cnt++;
                                cs->stop_selection = true;
                                cs->evaluating = true;
                                eval_batch.push_back(cs);
                                dd++;
                            }
                        }
                        if (cur_cnt >= target_cnt) break;
                    }
                    cur_leaf_batch_count++;
                }

                State* next = action->child_state.value().get();
                int ns_before = next->ns;
                action->visit++;
                int ns_after = next->ns = max(next->ns, action->visit);

                if (ns_before == ns_after) {
                    use_transposition = true;
                }

                current = next;
            }

            State* leaf = _sim_stack.back();
            if (leaf->stop_selection) global_stat.node_evaluated_used++;
            leaf->stop_selection = false;

            leaf->dv(_timeline.time()) = (backup_v - leaf->v) / leaf->ns;
            unordered_set<State*> ls;
            ls.insert(leaf);

            if (evaluating_node_visited || cur_leaf_batch_count == _config.leaf_batch_count || i_sim == k) {
                if (eval_batch.size() > 0) {
                    global_stat.eval_batch_count++;
                    global_stat.node_evaluated_total += eval_batch.size();
                    vector<TGame*> games;
                    for (State* s : eval_batch) {
                        games.push_back(&s->game);
                    }

                    vector<Evaluation<TGame>> evals = _evaluator->evaluate(games, _rnd_eng);
                    assert(evals.size() == eval_batch.size());
                    for (int i = 0; i < evals.size(); i++) {
                        State* s = eval_batch[i];
                        s->evaluated = true;
                        s->evaluating = false;
                        s->eval = make_unique<Evaluation<TGame>>(std::move(evals[i]));
                        assert(s->v == 0.0f);
                        s->dv(_timeline.time()) = s->eval->v;
                        ls.insert(s);
                    }
                }
                eval_batch.clear();
                cur_leaf_batch_count = 0;
                ts_last_batch = _timeline.time();
            }


            // 纠正路径上的v（由ns+1引起）
            for (int i = 0; i < _sim_stack.size() - 1; i++) {
                State* cur = _sim_stack[i];
                State* next = _sim_stack[i + 1];
                cur->dv(_timeline.time()) += (cur->convert_v(next) - cur->v) / cur->ns;
            }


            vector<State*> lsv;
            for (auto l : ls) lsv.push_back(l);
            backup_dv(lsv);

        }

        global_stat.tt_load_factor = _transposition.load_factor();
        global_stat.v_a = get_value(Color::A);
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::move(Move<TGame> move) {
        TGame g = _root->game;
        g.move(move);
        _root = create_state(g, true);

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
}