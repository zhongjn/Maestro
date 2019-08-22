#include "game_gomoku.h"

using namespace Maestro;

void Maestro::Gomoku::set_check_interval(int d, int& start, int& end) {
    if (d == 0) {
        start = 0;
        end = BOARD_SIZE - 1;
    }
    else if (d == -1) {
        start = 4;
        end = BOARD_SIZE - 1;
    }
    else if (d == 1) {
        start = 0;
        end = BOARD_SIZE - 5;
    }
    else {
        assert(0);
    }
}

bool Maestro::Gomoku::check_dir(const HalfBoard& hb, int dr, int dc) {
    int r_start, r_end, c_start, c_end;
    set_check_interval(dr, r_start, r_end);
    set_check_interval(dc, c_start, c_end);

    for (int r = r_start; r <= r_end; r++) {
        for (int c = c_start; c <= c_end; c++) {
            bool connect = true;
            for (int k = 0; k < 5; k++) {
                connect &= hb.get(r + k * dr, c + k * dc);
                if (!connect) break;
            }
            if (connect) connect = SIX_WIN || (!hb.safe_get(r - dr, c - dc) && !hb.safe_get(r + 5 * dr, c + 5 * dc));
            if (connect) return true;
        }
    }
    return false;
}

void Maestro::Gomoku::check_status() {
    for (int cc = 0; cc <= 1; cc++) {
        const HalfBoard& hb = cc == 0 ? black : white;
        Color cur_color = cc == 0 ? Color::A : Color::B;

        if (check_dir(hb, 1, 1) ||
            check_dir(hb, 1, -1) ||
            check_dir(hb, 0, 1) ||
            check_dir(hb, 1, 0)) {
            _status.end = true;
            _status.winner = cur_color;
            return;
        }
    }

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            // if not filled
            if (!black.get(r, c) && !white.get(r, c)) {
                _status.end = false;
                _status.winner = Color::None;
                return;
            }
        }
    }

    // filled
    _status.end = true;
    _status.winner = Color::None;
}

void Maestro::Gomoku::move(Move<Gomoku> mov) {
    if (!is_legal_move(mov)) throw logic_error("not a legal move");
    _steps++;
    assert(!_status.end);
    assert(is_legal_move(mov));
    _last_move = mov;

    if (_color == Color::A) {
        black.set(mov, true);
    }
    else if (_color == Color::B) {
        white.set(mov, true);
    }
    _color = another_color(_color);

    check_status();
}

vector<Move<Gomoku>> Maestro::Gomoku::get_all_legal_moves() const {
    vector<Move<Gomoku>> ms;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            Move<Gomoku> m{ r,c };
            if (is_legal_move(m)) ms.push_back(m);
        }
    }
    return ms;
}

string Maestro::Gomoku::to_string() const {
    ostringstream out;
    out << "step " << _steps << endl;
    const int cell_size = 3;
    const int row_size = (cell_size + 1) * BOARD_SIZE + 1;
    char line[row_size + 1];
    char line2[row_size + 1];
    for (int i = 0; i < row_size; i++) {
        if (i % (cell_size + 1) == 0)
            line[i] = '+';
        else
            line[i] = '-';

        line2[i] = ' ';
    }
    line[row_size] = 0;
    line2[row_size] = 0;
    out << line << endl;
    for (int i = 0; i < BOARD_SIZE; i++) {
        // char row[row_size];  // '  O  '
        for (int j = 0; j < BOARD_SIZE; j++) {
            line2[j * (cell_size + 1)] = '|';
            if (_last_move.row == i) {
                if (_last_move.col == j) {
                    line2[j * (cell_size + 1)] = '[';
                }
                else if (_last_move.col == j - 1) {
                    line2[j * (cell_size + 1)] = ']';
                }
            }

            int st = j * (cell_size + 1) + cell_size / 2 + 1;
            if (black.get(i, j))
                line2[st] = '#';
            else if (white.get(i, j))
                line2[st] = 'O';
            else
                line2[st] = ' ';
        }
        line2[row_size - 1] = '|';
        if (_last_move.row == i && _last_move.col == BOARD_SIZE - 1) {
            line2[row_size - 1] = ']';
        }

        out << line2 << " " << i + 1 << endl;
        out << line << endl;
    }

    char column_no[row_size] = { 0 };
    int offset = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        offset += sprintf_s(column_no + offset, row_size - offset, "  %-2c", char('A' + i));
    }
    out << column_no;
    return out.str();
}
