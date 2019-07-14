#include "MCT.h"
#include <cmath>

MCTNode::MCTNode(MCTNode * parent, double P, double V, GState * state)
{
	m_N = 1;				// 0 or 1? or it doesn't matter
	m_P = P;
	m_Q = V;
	m_V = V;
	m_totalV = V;
	m_parent = parent;
	m_state = state;
}

MCTNode::~MCTNode()
{
	for (std::map<GAction*, MCTNode*>::iterator iter = m_map.begin(); iter != m_map.end(); ++iter)
	{
		if (iter->first)
		{
			delete iter->first;
		}
		if (iter->second)
		{
			delete iter->second;
		}
	}
	if (m_state)
	{
		delete m_state;
	}
}

MCTNode * MCTNode::select_best(double kucb)
{
	MCTNode* ret = nullptr;
	double max;
	for (std::map<GAction*, MCTNode*>::iterator iter = m_map.begin(); iter != m_map.end(); ++iter)
	{
		double ucb = iter->second->cal_UCB(kucb);
		if (ret == nullptr)
		{
			max = ucb;
			ret = iter->second;
		}
		else if (max < ucb)
		{
			max = ucb;
			ret = iter->second;
		}
	}
	return ret;
}

int MCTNode::expand(std::map<GAction*, std::pair<double, double>> actions)
{
	for (std::map<GAction*, std::pair<double, double>>::iterator iter = actions.begin();
		iter != actions.end(); ++iter)
	{
		if (m_map.find(iter->first) == m_map.end())
		{
			MCTNode* new_node = new MCTNode(this, iter->second.first,
				iter->second.second, m_state->get_next_state(*(iter->first)));
			// May be improved by emplace
			m_map.insert(std::pair<GAction*, MCTNode*>(iter->first, new_node));
			State V = new_node->m_state->m_result;
			if (V != State::NOTEND)
			{
				// terminated
				new_node->m_Q = new_node->m_V = new_node->m_totalV = int(V);
			}

			new_node->backup();
		}
	}
	return 0;
}

void MCTNode::backup()
{
	MCTNode* pcur = m_parent;
	int V = -m_V;
	while (pcur != nullptr)
	{
		pcur->m_totalV += V;
		pcur->m_Q = pcur->m_totalV / pcur->m_N;
		pcur = pcur->m_parent;
		V = -V;
	}
}

MCT::MCT(GState * init_state, double t, double kpi, double kucb)
{
	m_ptrcur = m_root = new MCTNode(nullptr, 1, 0, init_state);
	// TODO: temperature should be selected carefully
	m_t = t;
	m_kpi = kpi;
	m_kucb = kucb;
}

void MCT::simulate(std::function<std::map<GAction*, std::pair<double, double>>(GState*)> NN)
{
	MCTNode* pcur = m_ptrcur;
	// simulation
	while (pcur->m_state->m_result == State::NOTEND)
	{
		pcur->m_N++;
		if (pcur->m_map.size() == 0)
		{
			pcur->expand(NN(pcur->m_state));
			// backup is done in expand
		}
		pcur = pcur->select_best(m_kucb);
	}
}

std::map<GAction*, double> MCT::get_probabilities()
{
	std::map<GAction*, double> ret;
	for (std::map<GAction*, MCTNode*>::iterator iter = m_ptrcur->m_map.begin();
		iter != m_ptrcur->m_map.end(); ++iter)
	{
		ret.insert({ iter->first, m_kpi * pow(iter->second->m_N, (double)1 / m_t) });
	}
	return ret;
}

void MCT::move(GAction * a)
{
	std::map<GAction*, MCTNode*>::iterator next = m_ptrcur->m_map.find(a);
	if (next != m_ptrcur->m_map.end())
	{
		m_ptrcur = next->second;
		m_ptrcur->m_parent->m_map.erase(next);
		delete m_ptrcur->m_parent;
		m_ptrcur->m_parent = nullptr;
	}
	else
	{
		// invalid move
		throw;
	}
}
