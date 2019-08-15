#pragma once
#include "search_base.h"

namespace Maestro {
	template<typename TGame>
	class MCTSNode {
	public:
		MCTSNode(MCTSNode<TGame>* p, MovePrior<TGame> m) {
			m_parent = p;
			m_move = m;
			m_N = m_Q = m_W = 0;
            m_expanded = false;
            m_game = nullptr;
		}

		~MCTSNode() {
			for (MCTSNode<TGame>*& child : m_children) {
				if (child) {
					delete child;
				}
			}
            if (m_game) {
                delete m_game;
            }
		}

		MCTSNode* select_best(float kucb) {
			MCTSNode* ret = nullptr;
			float max;
			for (MCTSNode<TGame>*& child : m_children) {
				float ucb = child->cal_UCB(kucb);
				if (ret == nullptr) {
					max = ucb;
					ret = child;
				} else if (max < ucb) {
					max = ucb;
					ret = child;
				}
			}
			return ret;
		}

		MCTSNode* select_best_with_diri(float kucb, const vector<float>& noise) {
			MCTSNode* ret = nullptr;
			float max;
			for (int i = 0; i < m_children.size(); ++i) {
                // 参数: epsilon
				float ucb = m_children[i]->cal_UCB(kucb, noise[i], 0.25);
				if (ret == nullptr) {
					max = ucb;
					ret = m_children[i];
				} else if (max < ucb) {
					max = ucb;
					ret = m_children[i];
				}
			}
			return ret;
		}

		void expand(Evaluation<TGame>&& eval) {
			if (!m_expanded) {
				Status s = m_game->get_status();
				if (s.end) {
					backup(m_game->get_color() != s.winner ? 1 : -1);
				} else {
					backup(eval.v);
					for (MovePrior<TGame>& p : eval.p) {
						MCTSNode* new_node = new MCTSNode(this, p);
						m_children.push_back(new_node);
					}
				}
                m_expanded = true;
			}
		}

		inline float cal_UCB(float k, float eta = 0, float epsilon = 0) {
			return m_Q + ((1 - epsilon) * m_move.p + epsilon * eta) 
				* k * sqrt(m_parent->m_N) / (1 + m_N);
		}

		void backup(float v) {
			MCTSNode* pcur = m_parent;
			v = -v;
			while (pcur != nullptr) {
				pcur->m_N++;
				pcur->m_W += v;
				pcur->m_Q = pcur->m_W / pcur->m_N;
				pcur = pcur->m_parent;
				v = -v;
			}
		}

		MCTSNode<TGame>* m_parent;					// parent node
		std::vector<MCTSNode<TGame>*> m_children;	// children
		TGame* m_game;								// game state, only available in leaf
		MovePrior<TGame> m_move;					// move and prior probability
		int m_N;									// visit cnt
		float m_Q;									// action value Q
		float m_W;									// sum of V in subtree, used in calculating Q
        bool m_expanded;
	};

	template<typename TGame>
	class MonteCarloTreeSearch final : public IMonteCarloSearch<TGame> {
	public:
        // 参数: kucb
		MonteCarloTreeSearch(TGame* init, float kucb, IEvaluator<TGame>* evaluator) {
			m_root = new MCTSNode<TGame>(nullptr, MovePrior<TGame>());
            m_root->m_game = init;
			m_kucb = kucb;
			m_evaluator = evaluator;
            
            m_root->expand(m_evaluator->evaluate(*(m_root->m_game)));
            root_expand2();
		}

        // 参数: k次迭代
		void simulate(int k) {
			for (int i = 0; i < k; ++i) {
				MCTSNode<TGame>* pcur = m_root;
				while (!pcur->m_game->get_status().end) {
					if (!pcur->m_expanded) {
						pcur->expand(m_evaluator->evaluate(*pcur->m_game));
					}
                    if (pcur == m_root) {
                        pcur = pcur->select_best_with_diri(m_kucb, m_noise);
                    } else {
					    pcur = pcur->select_best(m_kucb);
                    }
                    pcur->m_game = new TGame(*(pcur->m_parent->m_game));
                    pcur->m_game->move(pcur->m_move.move);
				}
			}
		}

		vector<MoveVisit> get_moves() const {
			vector<MoveVisit> ret;
			for (MCTSNode<TGame>*& child : m_root->m_children) {
				ret.push_back(MoveVisit{ child->m_move.move, child->m_N });
			}
			return ret;
		}

		float get_value(Color color) const {
            // 如果玩家不同，取相反数
			return m_root->m_game->get_color() == color ? m_root->m_Q : -m_root->m_Q;
		}

		TGame get_game_snapshot() const {
			return *(m_root->m_game);
		}

		void move(Move<TGame> move) {
			std::vector<MCTSNode<TGame>*>::iterator iter = m_root->m_children.begin();
			for (; iter != m_root->m_children.end(); ++iter) {
				if ((*iter)->m_move.move == move) {
					break;
				}
			}
			if (iter != m_root->m_children.end()) {
				m_root = *iter;
				m_root->m_parent->m_children.erase(iter);
                if (!m_root->m_game) {
                    m_root->m_game = new TGame(*(m_root->m_parent->m_game));
                    m_root->m_game->move(m_root->m_move.move);
                }
				delete m_root->m_parent;
				m_root->m_parent = nullptr;
			} else {
				// move doesn't exist
				throw;
			}
            root_expand2();
		}
	private:
        // Fully expand root and generate dirichlet noise
        void root_expand2() {
            vector<Move<TGame>> moves = m_root->m_game->get_all_legal_moves();
            int children_size = m_root->m_children.size();

            for (Move<TGame>& m : moves) {
                bool found = false;
                for (int i = 0; i < children_size; ++i) {
                    if (m_root->m_children[i]->m_move.move == m) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    MCTSNode<TGame>* new_node =
                        new MCTSNode<TGame>(m_root, MovePrior<TGame>{m, 0});
                    m_root->m_children.emplace_back(new_node);
                }
            }
            // 参数: Dir(0.03)
            m_noise = rand_dirichlet(m_root->m_children.size(), 0.03);
        }

        std::vector<float> rand_dirichlet(int n, float concentration)
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

		MCTSNode<TGame>* m_root;
		IEvaluator<TGame>* m_evaluator;
		float m_kucb;
        vector<float> m_noise;
	};
}

