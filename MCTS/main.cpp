#include <iostream>
#include "gomoku.h"
#include "MCT.h"

using namespace std;

std::vector<NNPair> NN(GState* s);

int main(void)
{
	GState* init_state = new GState();
	// TODO: 这里有三个参数要调
	MCT tree = MCT(init_state, 1, 1, 1);
	int c = 1;
	
	while (tree.cur_state()->m_result == State::NOTEND)
	{
		for (int i = 0; i < 1000; ++i)
		{
			tree.simulate(NN);
		}
		vector<PiPair> pi = tree.get_pi();
		GAction* opt_a = nullptr;
		double max_p;
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
		tree.move(opt_a);
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

std::vector<NNPair> NN(GState* s)
{
	Color cur_player = Color::BLANK;
	if (s->m_cur_played == Color::WHITE)
	{
		cur_player = Color::BLACK;
	}
	else if (s->m_cur_played == Color::BLACK)
	{
		cur_player = Color::WHITE;
	}
	else
	{
		cur_player = s->m_last_played;
	}

	std::vector<NNPair> ret;
	GState stemp(*s);
	GAction a = GAction(0, 0, Color::BLANK);
	int b, w;
	judge(stemp.m_board, b, w);
	int max = cur_player == Color::BLACK ? b : w;
	ret.push_back(NNPair(new GAction(a), max, max > 3 ? 1 : 0));

	a.m_color = cur_player;
	for (a.m_x = 0; a.m_x < NROWS; ++a.m_x)
	{
		for (a.m_y = 0; a.m_y < NCOLS; ++a.m_y)
		{
			if (s->m_board[a.m_x][a.m_y] == Color::BLANK)
			{
				s->get_next_state(stemp, a);
				judge(stemp.m_board, b, w);
				int max = cur_player == Color::BLACK ? b : w;
				ret.push_back(NNPair(new GAction(a), max, max > 3 ? 1 : 0));
			}
		}
	}
	return ret;
}

