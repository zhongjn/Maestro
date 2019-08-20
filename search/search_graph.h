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

    const int SPECULATIVE_BATCH_SIZE = 4;
    // const float PUCT = 2;
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

            State(Transposition* tt, TGame game) : game(game) {
                child_actions = [this, tt]() {
                    vector<shared_ptr<Action>> actions;
                    assert(eval);
                    for (auto& mp : eval->p) {
                        actions.push_back(make_shared<Action>(tt, mp.move, weak_from_this(), mp.p));
                    }
                    eval = nullptr;
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

        // hyper parameters
        int _leaf_batch_count;
        float _puct, _virtual_loss;

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



        bool _dirichlet_noise;

        vector<State*> _sim_stack;
        vector<State*> _backup_stack;

        float action_ucb(State* parent, Action* action, int ts_virtual_loss);

        void backup_dv(const vector<State*>& origins, Timeline tl = Timeline::origin);

        void slow_dfs_traversal(function<void(State*)> fn);

        vector<float> rand_dirichlet(int n, float concentration);

        void generate_root_dirichlet_noise();

        void collect_children_evaluated_dist();

    public:

        MonteCarloGraphSearch(shared_ptr<IEvaluator<TGame>> evaluator, TGame game, bool dirichlet_noise = false, int leaf_batch_count = 8, float puct = 2, float virtual_loss = 1) :
            _evaluator(std::move(evaluator)),
            _dirichlet_noise(dirichlet_noise),
            _leaf_batch_count(leaf_batch_count),
            _puct(puct),
            _virtual_loss(virtual_loss)
        {
            _root = make_shared<State>(&_transposition, game);
        }

        virtual void simulate(int k);

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

        virtual void move(Move<TGame> move);
    };

    template<typename TGame>
    inline float MonteCarloGraphSearch<TGame>::action_ucb(State* parent, Action* action, int ts_virtual_loss) {
        static uniform_real_distribution<float> dist(0, 1E-3);
        float u = 0;
        if (action->child_state.initialized()) {
            State* child = action->child_state.value().get();
            u += parent->convert_v(child);
            u -= child->virtual_loss_cnt(ts_virtual_loss) * _virtual_loss / max(1, child->ns);
        }
        u += dist(_rnd_eng);
        u += _puct * (action->p + action->p_noise_delta) * sqrtf(parent->ns) / (1 + action->visit);
        return u;
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::backup_dv(const vector<State*> & origins, Timeline tl) {

        int visit_expect = 0, visit_actual = 0;
        int ts = _timeline.time(tl);

        // 1. ������DAG���ڵ㺢������
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

        // 2. ����DAG����������������������ӽڵ㿪ʼ��������dv
        _backup_stack.clear();
        for (auto o : origins) {
            _backup_stack.push_back(o);
        }
        while (!_backup_stack.empty()) {
            State* cur = _backup_stack.back();
            _backup_stack.pop_back();
            ++visit_actual;

            float cur_v_before = cur->v;
            // �ۼ�dv��v��
            cur->v += cur->dv(ts);
            float cur_v_after = cur->v;

            for (auto& wpa : cur->parent_actions) {
                shared_ptr<Action> pa = wpa.lock();
                if (pa) {
                    shared_ptr<State> ps = pa->parent_state.lock();
                    assert(ps);
                    Color cur_color = cur->game.get_color();

                    // ����״̬dv�������ǰdv���ۼ�
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
            ret[i] = gamma(_rnd_eng);
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
                auto ac = make_shared<Action>(&_transposition, m, weak_ptr(_root), 0);
                action = ac.get();
                _root->child_actions->push_back(ac);
            }
            action->p_noise_delta = -NOISE_EPSILON * action->p + NOISE_EPSILON * noise[i];
            ++i;
        }
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::collect_children_evaluated_dist() {
        global_stat.children_evaluated_dist.clear();
        map<int, int> dist;
        int max_count = 0;
        slow_dfs_traversal([&dist, &max_count](State * state) {
            if (state->ns > 0 && state->child_actions.initialized()) {
                int count = 0;
                for (auto& ac : state->child_actions.value()) {
                    if (ac->child_state.initialized()) {
                        if (ac->child_state.value()->evaluated && ac->child_state.value()->ns > 0) {
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

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::simulate(int k) {
        global_stat = GlobalStat();

        int cur_leaf_batch_count = 0;
        vector<State*> eval_batch;

        int ts_last_batch = 0;

        for (int i_sim = 1; i_sim <= k; i_sim++) {
            bool evaluating_node_visited = false;

            global_stat.sim_total++;
            _timeline.next_epoch();
            _sim_stack.clear();

            _root->ns++;
            // ���ڵ���Ҫ���⴦��
            if (!_root->evaluated) {
                _root->eval = make_unique<Evaluation<TGame>>(_evaluator->evaluate(_root->game));
                _root->evaluated = true;
                _root->v = _root->eval->v;
                continue;
            }

            if (_dirichlet_noise) {
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
                        throw logic_error("?");
                        assert(0);
                    }
                }

                vector<shared_ptr<Action>>& actions = current->child_actions.value();
                vector<float> ac_ucb_temp;
                ac_ucb_temp.reserve(actions.size());
                Action* action = nullptr;
                int max_idx = -1;
                float max_ucb = -100000;
                int idx = 0;
                for (auto& ac : actions) {
                    float ucb = action_ucb(current, ac.get(), ts_last_batch);
                    ac_ucb_temp.push_back(ucb);
                    if (ucb > max_ucb) {
                        max_ucb = ucb;
                        max_idx = idx;
                        action = ac.get();
                    }
                    idx++;
                }

                ac_ucb_temp[max_idx] += 1000; // ��Ҫ����֤ѡ�е�action��������ǰ

                if (!action->child_state.initialized() && action->child_state.value().get()->ns == 0) {
                    // ����ǰk��δ��ʼ�����ӽڵ�
                    // �����ֵ���У�������Ϊevaluating
                    vector<int> ac_no;
                    ac_no.reserve(actions.size());
                    int idx = 0;
                    for (auto& ac : actions) {
                        ac_no.push_back(idx++);
                    }
                    int n_select = min<int>(ac_no.size(), SPECULATIVE_BATCH_SIZE);
                    // TODO: �����ö����Ż�
                    sort(ac_no.begin(), ac_no.end(), [&ac_ucb_temp](int no1, int no2) { return ac_ucb_temp[no1] > ac_ucb_temp[no2]; });
                    
                    int cur_cnt = 0;
                    int t = 0;
                    for (int no : ac_no) {
                        if (actions[no]->child_state.initialized()) {
                            State* cs = actions[no]->child_state.value().get();
                            if (cs->ns > 0) {
                                t++;
                            }
                        }
                    }

                    int target_cnt = clamp(t * 1, 1, 20000);

                    for (int no : ac_no) {
                        State* cs = actions[no]->child_state.value().get();
                        if (cs->ns == 0) {
                            if (!cs->game.get_status().end && !cs->evaluating && !cs->evaluated) {
                               
                                cur_cnt++;
                                cs->stop_selection = true;
                                cs->evaluating = true;
                                eval_batch.push_back(cs);
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
            leaf->dv(_timeline.time()) += (backup_v - leaf->v) / leaf->ns;

            // ����·���ϵ�v����ns+1����
            for (int i = 0; i < _sim_stack.size() - 1; i++) {
                State* cur = _sim_stack[i];
                State* next = _sim_stack[i + 1];
                cur->dv(_timeline.time()) += (cur->convert_v(next) - cur->v) / cur->ns;
            }

            vector<State*> ls;
            ls.push_back(leaf);
            backup_dv(ls);

            if (evaluating_node_visited || cur_leaf_batch_count == _leaf_batch_count || i_sim == k) {
                if (eval_batch.size() > 0) {
                    global_stat.eval_batch_count++;
                    global_stat.node_evaluated_total += eval_batch.size();
                    vector<TGame*> games;
                    for (State* s : eval_batch) {
                        games.push_back(&s->game);
                    }
                    vector<Evaluation<TGame>> evals = _evaluator->evaluate(games);
                    assert(evals.size() == eval_batch.size());
                    for (int i = 0; i < evals.size(); i++) {
                        State* s = eval_batch[i];
                        s->evaluated = true;
                        s->evaluating = false;
                        s->eval = make_unique<Evaluation<TGame>>(std::move(evals[i]));
                        assert(s->v == 0.0f);
                        s->dv(_timeline.time(Timeline::backup_eval)) = s->eval->v;
                    }

                    backup_dv(eval_batch, Timeline::backup_eval);
                }
                eval_batch.clear();
                cur_leaf_batch_count = 0;
                ts_last_batch = _timeline.time();
            }
        }

        global_stat.tt_load_factor = _transposition.load_factor();
    }

    template<typename TGame>
    inline void MonteCarloGraphSearch<TGame>::move(Move<TGame> move) {
        TGame g = _root->game;
        g.move(move);
        _root = make_shared<State>(&_transposition, g);

        if (_dirichlet_noise) {
            generate_root_dirichlet_noise();
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
}