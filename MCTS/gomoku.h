#pragma once
#include <map>
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
	GAction(int x, int y, Color color);
	~GAction() = default;
	int m_x;
	int m_y;
	Color m_color;
};

class GState
{
public:
	GState();
	GState(GState& s);
	~GState() = default;

	GState* get_next_state(const GAction& a);
	void get_next_state(GState& s, const GAction& a);
	void check_win();
	std::string to_string();

	Color m_board[NROWS][NCOLS];
	State m_result;
	Color m_cur_played;
	Color m_last_played;
};


std::pair<int, int> judge(Color board[NROWS][NCOLS]);
std::map<GAction*, std::pair<double, double>> NN(GState* s);
