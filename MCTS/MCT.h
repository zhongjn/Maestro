#pragma once
#include <functional>
#include <vector>
#include "gomoku.h"

class MCTNode
{
public:
	MCTNode(MCTNode* parent, float p, GAction* a, GState* s);
	~MCTNode();

	MCTNode* select_best(float kucb);
	int expand(std::vector<NNPair>&& actions);

	std::vector<MCTNode*> m_children;	// children
	MCTNode* m_parent;					// parent node
	GAction* m_action;					// action
	GState* m_state;					// game state
	int m_N;							// visit cnt
	float m_P;							// prior probability
	float m_Q;							// action value Q
	float m_W;							// sum of V in subtree, used in calculating Q

private:
	inline float cal_UCB(float k)
	{
		return m_Q + m_P * k * sqrt(m_parent->m_N) / (1 + m_N);
	}
	void backup(float v);

};

struct PiPair
{
	PiPair(GAction* a, float p)
	{
		m_a = a;
		m_p = p;
	}

	// m_a may be deleted after take move
	GAction* m_a;
	float m_p;
};

class MCT
{
public:
	MCT(GState* init_state, float t, float kpi, float kucb);
	void simulate(std::function<std::vector<NNPair>(GState*)> NN);
	// return pi
	std::vector<PiPair> get_pi();
	void take_action(GAction* a);
	inline GState* cur_state()
	{
		return m_root->m_state;
	}
	inline void set_t(float t)
	{
		m_t = t;
	}

private:
	MCTNode* m_root;
	float m_t;
	float m_kpi;
	float m_kucb;
};