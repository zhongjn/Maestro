#include "gomoku.h"

GState* GState::get_next_state(const GAction& a)
{
	GState* ret = nullptr;
	if (m_board[a.m_x][a.m_y] == Color::BLANK)
	{
		ret = new GState(*this);
		ret->m_board[a.m_x][a.m_y] = a.m_color;
		ret->m_cur_played = a.m_color;
		ret->check_win();
	}

	return ret;
}

void GState::get_next_state(GState& s, const GAction& a)
{
	if (m_board[a.m_x][a.m_y] == Color::BLANK)
	{
		s = GState(*this);
		s.m_board[a.m_x][a.m_y] = a.m_color;
		s.m_cur_played = a.m_color;
		s.check_win();
	}
}

void GState::check_win()
{
	int w, b;
	if (judge(m_board, b, w) == 1)
	{
		m_result = State::TIE;
	}
	else if ((b == 5 && m_cur_played == Color::BLACK) || (w == 5 && m_cur_played == Color::WHITE))
	{
		m_result = State::WIN;
	}
	else if ((b == 5 && m_cur_played == Color::WHITE) || (w == 5 && m_cur_played == Color::BLACK))
	{
		m_result = State::LOSE;
	}
	else
	{
		m_result = State::NOTEND;
	}
}

std::string GState::to_string()
{
	std::string ret = "";
	for (int i = 0; i < NROWS; ++i)
	{
		for (int j = 0; j < NCOLS; ++j)
		{
			if (m_board[i][j] == Color::WHITE)
			{
				ret += 'O';
			}
			else if (m_board[i][j] == Color::BLACK)
			{
				ret += '@';
			}
			else
			{
				ret += '-';
			}
		}
		ret += '\n';
	}
	return ret;
}

int judge(Color board[NROWS][NCOLS], int& b, int& w)
{
	Color winner = Color::BLANK;
	int count = 0;
	int max_white = 0;
	int max_black = 0;
	Color color;

	// vertical
	for (int y = 0; y < NCOLS; ++y)
	{
		color = board[0][y];
		count = 1;

		for (int x = 1; x < NROWS; ++x)
		{
			if (board[x][y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[x][y];
				count = 1;
			}
		}
	}

	// horizontal
	for (int x = 0; x < NROWS; ++x)
	{
		color = board[x][0];
		count = 1;

		for (int y = 1; y < NCOLS; ++y)
		{
			if (board[x][y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[x][y];
				count = 1;
			}
		}
	}

	// right up triangle
	for (int y = 0; y < NCOLS - 4; ++y)
	{
		color = board[0][y];
		count = 1;

		for (int pos_x = 1, pos_y = y + 1; pos_x < NROWS && pos_y < NCOLS; ++pos_x, ++pos_y)
		{
			if (board[pos_x][pos_y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[pos_x][pos_y];
				count = 1;
			}
		}
	}

	// left down triangle
	for (int x = 0; x < NROWS - 4; ++x)
	{
		color = board[x][0];
		count = 1;

		for (int pos_x = x + 1, pos_y = 1; pos_x < NROWS && pos_y < NCOLS; ++pos_x, ++pos_y)
		{
			if (board[pos_x][pos_y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[pos_x][pos_y];
				count = 1;
			}
		}
	}

	// left up triangle
	for (int y = 4; y < NCOLS; ++y)
	{
		color = board[0][y];
		count = 1;

		for (int pos_x = 1, pos_y = y - 1; pos_x < NROWS && pos_y >= 0; pos_x++, pos_y--)
		{
			if (board[pos_x][pos_y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[pos_x][pos_y];
				count = 1;
			}
		}
	}

	// right down triangle
	for (int x = 0; x < NCOLS - 4; ++x)
	{
		color = board[x][NROWS - 1];
		count = 1;

		for (int pos_x = 1, pos_y = NROWS - 2; pos_x < NROWS && pos_y >= 0; pos_x++, pos_y--)
		{
			if (board[pos_x][pos_y] == color)
			{
				count++;
				if (color != Color::BLANK && count >= 5)
				{
					winner = color;
					goto result;
				}
			}
			else
			{
				if (color == Color::WHITE && count > max_white)
				{
					max_white = count;
				}
				else if (color == Color::BLACK && count > max_black)
				{
					max_black = count;
				}
				color = board[pos_x][pos_y];
				count = 1;
			}
		}
	}


result:
	if (winner == Color::BLACK)
	{
		max_black = 5;
	}
	else if (winner == Color::WHITE)
	{
		max_white = 5;
	}
	b = max_black;
	w = max_white;

	for (int i = 0; i < NROWS; ++i)
	{
		for (int j = 0; j < NCOLS; ++j)
		{
			if (board[i][j] == Color::BLANK)
			{
				return 0;
			}
		}
	}
	return 1;
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

	std::vector<NNPair> ret;
	GState stemp(*s);
	GAction a = GAction(0, 0, cur_player);
	int b, w;
	int max;
	float sum = 0;

	for (a.m_x = 0; a.m_x < NROWS; ++a.m_x)
	{
		for (a.m_y = 0; a.m_y < NCOLS; ++a.m_y)
		{
			if (s->m_board[a.m_x][a.m_y] == Color::BLANK)
			{
				s->get_next_state(stemp, a);
				judge(stemp.m_board, b, w);
				int max = cur_player == Color::BLACK ? (b - w + 5) : (w - b + 5);
				sum += max;
				ret.push_back(NNPair(new GAction(a), max, ((float)max - 5) / 10));
			}
		}
	}

	for (int i = 0; i < ret.size(); ++i)
	{
		ret[i].m_p = ret[i].m_p / sum;
	}
	return ret;
}
