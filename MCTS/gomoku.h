#pragma once
#include <vector>
#include <string>

#define NROWS 15
#define NCOLS 15

enum class State
{
	WIN = 1,
	LOSE = -1,
	TIE = 0,
	NOTEND = -1000
};

enum class Color
{
	BLANK,
	BLACK,
	WHITE
};

class GAction
{
public:
	GAction(int x, int y, Color color)
	{
		m_x = x;
		m_y = y;
		m_color = color;
	}

	~GAction() = default;
	int m_x;
	int m_y;
	Color m_color;
};

class GState
{
public:
	GState()
	{
		for (int i = 0; i < NROWS; ++i)
		{
			for (int j = 0; j < NCOLS; ++j)
			{
				m_board[i][j] = Color::BLANK;
			}
		}
		m_cur_played = Color::WHITE;					// black first
		m_result = State::NOTEND;
	}

	GState(GState& s)
	{
		for (int i = 0; i < NROWS; ++i)
		{
			for (int j = 0; j < NCOLS; ++j)
			{
				m_board[i][j] = s.m_board[i][j];
			}
		}
		m_cur_played = s.m_cur_played;
		m_result = State::NOTEND;
	}

	~GState() = default;

	GState* get_next_state(const GAction& a);
	void get_next_state(GState& s, const GAction& a);
	void check_win();
	std::string to_string();

	Color m_board[NROWS][NCOLS];
	State m_result;
	Color m_cur_played;
};

// tie return 1, else return 0
int judge(Color board[NROWS][NCOLS], int& b, int& w);


struct NNPair
{
	GAction* m_a;
	double m_p;
	double m_v;
	NNPair(GAction* a, double p, double v)
	{
		m_a = a;
		m_p = p;
		m_v = v;
	}
};

std::vector<NNPair> NN(GState* s);
