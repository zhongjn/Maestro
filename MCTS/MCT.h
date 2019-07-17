#pragma once
#include <functional>
#include <vector>
#include "gomoku.h"

class MCTNode
{
public:
	MCTNode(MCTNode* parent, double p, GAction* a, GState* s);
	~MCTNode();

	MCTNode* select_best(double kucb);
	int expand(std::vector<NNPair>&& actions);

	std::vector<MCTNode*> m_children;	// children
	MCTNode* m_parent;					// parent node
	GAction* m_action;					// action
	GState* m_state;					// game state
	int m_N;							// visit cnt
	double m_P;							// prior probability
	double m_Q;							// action value Q
	double m_W;							// sum of V in subtree, used in calculating Q

private:
	inline double cal_UCB(double k)
	{
		return m_Q + m_P * k * sqrt(m_parent->m_N) / (1 + m_N);
	}
	void backup(double v);

};

struct PiPair
{
	PiPair(GAction* a, double p)
	{
		m_a = a;
		m_p = p;
	}

	// m_a may be deleted after take move
	GAction* m_a;
	double m_p;
};

class MCT
{
public:
	MCT(GState* init_state, double t, double kpi, double kucb);
	void simulate(std::function<std::vector<NNPair>(GState*)> NN);
	// return pi
	std::vector<PiPair> get_pi();
	void take_action(GAction* a);
	inline GState* cur_state()
	{
		return m_root->m_state;
	}
	inline void set_t(double t)
	{
		m_t = t;
	}

private:
	MCTNode* m_root;
	double m_t;
	double m_kpi;
	double m_kucb;
};