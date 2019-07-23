#pragma once
#include "search_base.h"

namespace Maestro {
	template<typename TGame>
	class MCTSNode {
	public:
		MCTSNode(MCTSNode<TGame>* p, MovePrior<TGame> m, TGame* g) {
			m_parent = p;
			m_move = m;
			m_game = g;
			m_expanded = false;
			m_N = m_Q = m_W = 0;
		}

		~MCTSNode() {
			for (MCTSNode<TGame>*& child : m_children) {
				if (child) {
					delete child;
				}
			}
		}

		MCTSNode* select_best(float kucb, bool diri = false) {
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

		void expand(Evaluation<TGame>&& eval) {
            // TODO @wsh: 修复编译错误
			if (!m_expanded) {
				Status s = m_game->get_status();
				if (s.end) {
					backup(m_game->get_player == s.winner ? 1 : -1);
				} else {
					backup(eval.v);
					for (MovePrior<TGame>& p : eval.p) {
						MCTSNode* new_node = new MCTSNode(this, p, new TGame(*m_game));
						m_children.push_back(new_node);
						new_node->m_game->move(p.move);
					}
				}
				m_expanded = true;
				delete m_game;
				m_game = nullptr;
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

		bool m_expanded;
		MCTSNode<TGame>* m_parent;					// parent node
		std::vector<MCTSNode<TGame>*> m_children;	// children
		TGame* m_game;								// game state
		MovePrior<TGame> m_move;					// move and prior probability
		int m_N;									// visit cnt
		float m_Q;									// action value Q
		float m_W;									// sum of V in subtree, used in calculating Q
	};

	template<typename TGame>
	class MonteCarloTreeSearch final : public IMonteCarloSearch<TGame> {
	public:
		MonteCarloTreeSearch(TGame* init, float kucb, IEvaluator<TGame>* evaluator) {
			m_root = new MCTSNode<TGame>(nullptr, MovePrior<TGame>(), init);
			m_root_game = TGame(*init);
			m_kucb = kucb;
			m_evaluator = evaluator;
		}

		void simulate(int k) {
			for (int i = 0; i < k; ++i) {
				MCTSNode<TGame>* pcur = m_root;
				// simulation
				while (!pcur->m_game->get_status().end) {
					if (!pcur->m_expanded) {
						pcur->expand(m_evaluator->evaluate(pcur->m_game->get_obsv(pcur->m_game->get_player())));
						// backup is done in expand
					}
					pcur = pcur->select_best(m_kucb);
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

		float get_value(Player player) const {
            // TODO @wsh: 如果玩家不同，取相反数
			return m_root->m_Q;
		}

		TGame get_game_snapshot() const {
			return m_root_game;
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
				delete* iter;
				m_root->m_parent = nullptr;
				m_root_game.move(move);
			} else {
				// move doesn't exist
				throw;
			}
		}
	private:
		MCTSNode<TGame>* m_root;
		IEvaluator<TGame>* m_evaluator;
		TGame m_root_game;
		float m_kucb;
	};
}

// https://www.cnblogs.com/yeahgis/archive/2012/07/15/2592698.html
float rand_gamma(float alpha) {
	float u, v;
	float x, y;
	float b, c;
	float w, z;
	bool accept = false;
	float t;

	if (alpha > 1.0) {
		/* Best's rejection algorithm XG for gamma random variates (Best, 1978) */
		b = alpha - 1;
		c = 3 * alpha - 0.75;
		do {
			u = rand() / RAND_MAX;
			v = rand() / RAND_MAX;

			w = u * (1 - u);
			y = sqrt(c / w) * (u - 0.5);
			x = b + y;
			if (x >= 0) {
				z = 64 * w * w * w * v * v;
				float zz = 1 - 2 * y * y / x;
				if (z - zz < 1e-10) {
					accept = true;
				} else {
					accept = false;
				}
				if (!accept) {
					float logz = log(z);
					float zzz = 2 * (b * log(x / b) - y);
					if (logz - zzz < 1e-10) {
						accept = true;
					} else {
						accept = false;
					}
				}
			}
		} while (!accept);
		return x;
	} else if (alpha == 1.0) {
		x = rand() / RAND_MAX;
		// return -log(1 - x);
		return -log(x);
	} else if (alpha < 1.0) {
		float dv = 0.0;
		float b = (exp(1.0) + alpha) / exp(1.0);
		do {
			float u1 = rand() / RAND_MAX;
			float p = b * rand() / RAND_MAX;
			float y;
			if (p > 1) {
				y = -log((b - p) / alpha);
				if (u1 < pow(y, alpha - 1)) {
					dv = y;
					break;
				}
			} else {
				y = pow(p, 1 / alpha);
				if (u1 < exp(-y)) {
					dv = y;
					break;
				}
			}
		} while (1);
		return dv;
	}
	return -1;
}

std::vector<float> rand_dirichlet(int n, float concentration) {
	std::vector<float> ret = std::vector<float>(n, 0);
	float sum = 0;
	for (int i = 0; i < n; ++i) {
		ret[i] = rand_gamma(concentration);
		sum += ret[i];
	}
	for (int i = 0; i < n; ++i) {
		ret[i] = ret[i] / sum;
	}
	return ret;
}
