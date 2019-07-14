#include "gomoku.h"

GAction::GAction(int x, int y, Color color)
{
	m_x = x;
	m_y = y;
	m_color = color;
}

GState::GState()
{
	for (int i = 0; i < NROWS; ++i)
	{
		for (int j = 0; j < NCOLS; ++j)
		{
			m_board[i][j] = Color::BLANK;
		}
	}
	m_cur_played = Color::WHITE;					// black first
	m_last_played = Color::BLACK;
	m_result = State::NOTEND;
}

GState::GState(GState & s)
{
	for (int i = 0; i < NROWS; ++i)
	{
		for (int j = 0; j < NCOLS; ++j)
		{
			m_board[i][j] = s.m_board[i][j];
		}
	}
	m_cur_played = s.m_cur_played;
	m_last_played = s.m_last_played;
	m_result = State::NOTEND;
}

GState * GState::get_next_state(const GAction& a)
{
	GState* ret = nullptr;
	if (a.m_color == Color::BLANK)
	{
		// blank means no move
		ret = new GState(*this);
		ret->m_last_played = ret->m_cur_played;
		ret->m_cur_played = Color::BLANK;
		ret->check_win();
	}
	else if (m_board[a.m_x][a.m_y] == Color::BLANK)
	{
		ret = new GState(*this);
		ret->m_board[a.m_x][a.m_y] = a.m_color;
		ret->m_last_played = ret->m_cur_played;
		ret->m_cur_played = a.m_color;
		ret->check_win();
	}
	else
	{
		// invalid move
		throw;
	}
	return ret;
}

void GState::get_next_state(GState& s, const GAction& a)
{
	if (a.m_color == Color::BLANK)
	{
		// blank means no move
		s = GState(*this);
		s.m_last_played = s.m_cur_played;
		s.m_cur_played = Color::BLANK;
		check_win();
	}
	else if (m_board[a.m_x][a.m_y] == Color::BLANK)
	{
		s = GState(*this);
		s.m_board[a.m_x][a.m_y] = a.m_color;
		s.m_last_played = s.m_cur_played;
		s.m_cur_played = a.m_color;
		check_win();
	}
	else
	{
		// invalid move
		throw;
	}
}

void GState::check_win()
{
	if (m_last_played == Color::BLANK && m_cur_played == Color::BLANK)
	{
		m_result = State::TIE;
		return;
	}

	std::pair<int, int> res = judge(m_board);
	if ((res.first == 5 && m_cur_played == Color::BLACK) || (res.second == 5 && m_cur_played == Color::WHITE))
	{
		m_result = State::WIN;
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

std::pair<int, int> judge(Color board[NROWS][NCOLS])
{
	Color winner = Color::BLANK;	// for not end
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
	return std::pair<int, int>(max_black, max_white);
}

std::map<GAction*, std::pair<double, double>> NN(GState* s)
{
	std::map<GAction*, std::pair<double, double>> ret;
	GState stemp(*s);
	GAction a = GAction(0, 0, Color::BLANK);
	std::pair<int, int> max_pair = judge(stemp.m_board);
	int max = s->m_cur_played == Color::BLACK ? max_pair.second : max_pair.first;
	ret.insert(std::pair<GAction*, std::pair<double, double>>(new GAction(a)
		, {max, max > 3 ? 1 : 0}));

	a.m_color = s->m_cur_played == Color::WHITE ? Color::BLACK : Color::WHITE;
	for (a.m_x = 0; a.m_x < NROWS; ++a.m_x)
	{
		for (a.m_y = 0; a.m_y < NCOLS; ++a.m_y)
		{
			if (s->m_board[a.m_x][a.m_y] == Color::BLANK)
			{
				s->get_next_state(stemp, a);
				max_pair = judge(stemp.m_board);
				int max = s->m_cur_played == Color::BLACK ? max_pair.second : max_pair.first;
				ret.insert(std::pair<GAction*, std::pair<double, double>>(new GAction(a)
					, { max, max > 3 ? 1 : 0 }));
			}
		}
	}
	return ret;
}
