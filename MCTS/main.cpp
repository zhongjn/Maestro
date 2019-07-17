#include <iostream>
#include "gomoku.h"
#include "MCT.h"

using namespace std;

int main(void)
{
	GState* init_state = new GState();
	// TODO: 这里有三个参数要调
	MCT tree = MCT(init_state, 1, 1, 1);
	int c = 1;
	
	while (tree.cur_state()->m_result == State::NOTEND)
	{
		for (int i = 0; i < 50; ++i)
		{
			tree.simulate(NN);
		}
		vector<PiPair> pi = tree.get_pi();
		GAction *opt_a = nullptr;
		float max_p;
		for (vector<PiPair>::iterator iter = pi.begin(); iter != pi.end(); ++iter)
		{
			if (!opt_a)
			{
				opt_a = iter->m_a;
				max_p = iter->m_p;
			}
			else if (iter->m_p > max_p)
			{
				opt_a = iter->m_a;
				max_p = iter->m_p;
			}
		}
		tree.take_action(opt_a);
		cout << "RUN " << c++;
		if (tree.cur_state()->m_cur_played == Color::BLACK)
		{
			cout << ": BLACK" << endl;
		}
		else if (tree.cur_state()->m_cur_played == Color::WHITE)
		{
			cout << ": WHITE" << endl;
		}
		else
		{
			cout << endl;
		}
		cout << tree.cur_state()->to_string() << endl;
	}
	

	if (tree.cur_state()->m_result == State::WIN)
	{
		if (tree.cur_state()->m_cur_played == Color::BLACK)
		{
			cout << "BLACK WIN" << endl;
		}
		else
		{
			cout << "WHITE WIN" << endl;
		}
	}
	else
	{
		cout << "TIE" << endl;
	}

	return 0;
}

