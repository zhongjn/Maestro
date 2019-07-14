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
		for (int i = 0; i < 10000; ++i)
		{
			tree.simulate(NN);
		}
		map<GAction*, double> pi = tree.get_probabilities();
		GAction* opt_a = nullptr;
		double max_p;
		for (map<GAction*, double>::iterator iter = pi.begin(); iter != pi.end(); ++iter)
		{
			if (!opt_a)
			{
				opt_a = iter->first;
				max_p = iter->second;
			}
			else if (iter->second > max_p)
			{
				opt_a = iter->first;
				max_p = iter->second;
			}
		}
		tree.move(opt_a);
		cout << "RUN " << c++;
		if (tree.cur_state()->m_cur_played == Color::BLACK)
		{
			cout << ": BLACK" << endl;
		}
		else
		{
			cout << ": WHITE" << endl;
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