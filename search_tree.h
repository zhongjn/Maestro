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
			for (std::vector<MCTSNode*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter) {
				if (*iter) {
					delete* iter;
				}
			}
		}

		MCTSNode* select_best(float kucb) {
			MCTSNode* ret = nullptr;
			float max;
			for (std::vector<MCTSNode*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter) {
				float ucb = (*iter)->cal_UCB(kucb);
				if (ret == nullptr) {
					max = ucb;
					ret = *iter;
				} else if (max < ucb) {
					max = ucb;
					ret = *iter;
				}
			}
			return ret;
		}

		void expand(Evaluation<TGame>&& eval) {
            // TODO @wsh: 修复编译错误
			if (!m_expanded) {
				Status s = m_game->get_status();
				if (s.end) {
					new_node->backup(new_node->m_game->get_player == s.winner ? 1 : -1);
				} else {
					new_node->backup(iter->m_v);
					for (std::vector<MovePrior<TGame>>::iterator iter = eval.p.begin();
							iter != eval.p.end(); ++iter) {
						MCTSNode* new_node = new MCTSNode(this, *iter, new TGame(*m_game));
						m_children.push_back(new_node);
						new_node->m_game->move(iter->move);
					}
				}
				m_expanded = true;
				delete m_game;
				m_game = nullptr;
			}
		}

		inline float cal_UCB(float k) {
			return m_Q + m_move.p * k * sqrt(m_parent->m_N) / (1 + m_N);
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
			for (std::vector<MCTSNode<TGame>*>::iterator iter = m_root->m_children.begin();
					iter != m_root->m_children.end(); ++iter) {
				ret.push_back(MoveVisit{ (*iter)->m_move.move, (*iter)->m_N });
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