#include "MCT.h"
#include <cmath>

MCTNode::MCTNode(MCTNode* parent, double p, GAction* a, GState* s)
{
	m_N = 0;
	m_P = p;
	m_Q = 0;
	m_W = 0;
	m_parent = parent;
	m_state = s;
	m_action = a;
}

MCTNode::~MCTNode()
{
	for (std::vector<MCTNode*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		if (*iter)
		{
			delete *iter;
		}
	}
	if (m_state)
	{
		delete m_state;
	}
	if (m_action)
	{
		delete m_action;
	}
}

MCTNode * MCTNode::select_best(double kucb)
{
	MCTNode* ret = nullptr;
	double max;
	for (std::vector<MCTNode*>::iterator iter = m_children.begin(); iter != m_children.end(); ++iter)
	{
		double ucb = (*iter)->cal_UCB(kucb);
		if (ret == nullptr)
		{
			max = ucb;
			ret = *iter;
		}
		else if (max < ucb)
		{
			max = ucb;
			ret = *iter;
		}
	}
	return ret;
}

int MCTNode::expand(std::vector<NNPair>&& actions)
{
	for (std::vector<NNPair>::iterator iter = actions.begin();
		iter != actions.end(); ++iter)
	{
		MCTNode* new_node = new MCTNode(this, iter->m_p,
			iter->m_a, m_state->get_next_state(*(iter->m_a)));
		m_children.push_back(new_node);
		State v = new_node->m_state->m_result;
		if (v != State::NOTEND)
		{
			new_node->backup(int(v));
		}
		else
		{
			new_node->backup(iter->m_v);
		}
	}
	return 0;
}

void MCTNode::backup(double v)
{
	MCTNode* pcur = m_parent;
	v = -v;
	while (pcur != nullptr)
	{
		pcur->m_N++;
		pcur->m_W += v;
		pcur->m_Q = pcur->m_W / pcur->m_N;
		pcur = pcur->m_parent;
		v = -v;
	}
}

MCT::MCT(GState * init_state, double t, double kpi, double kucb)
{
	m_root = new MCTNode(nullptr, 0, nullptr, init_state);
	m_t = t;
	m_kpi = kpi;
	m_kucb = kucb;
}

void MCT::simulate(std::function<std::vector<NNPair>(GState*)> NN)
{
	MCTNode* pcur = m_root;
	// simulation
	while (pcur->m_state->m_result == State::NOTEND)
	{
		if (pcur->m_children.size() == 0)
		{
			pcur->expand(NN(pcur->m_state));
			// backup is done in expand
		}
		pcur = pcur->select_best(m_kucb);
	}
}

std::vector<PiPair> MCT::get_pi()
{
	std::vector<PiPair> ret;
	for (std::vector<MCTNode*>::iterator iter = m_root->m_children.begin();
		iter != m_root->m_children.end(); ++iter)
	{
		ret.push_back(PiPair((*iter)->m_action, m_kpi * pow((*iter)->m_N, (double)1 / m_t)));
	}
	return ret;
}

void MCT::move(GAction * a)
{
	std::vector<MCTNode*>::iterator next = m_root->m_children.begin();
	for (; next != m_root->m_children.end(); ++next)
	{
		if ((*next)->m_action == a)
		{
			break;
		}
	}
	if (next != m_root->m_children.end())
	{
		m_root = *next;
		m_root->m_parent->m_children.erase(next);
		delete m_root->m_parent;
		m_root->m_parent = nullptr;
	}
	else
	{
		// invalid move
		throw;
	}
}
