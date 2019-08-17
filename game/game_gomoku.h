#pragma once
#include "game_base.h"
#include <cassert>
#include <bitset>
#include <sstream>

namespace Maestro {
    using namespace std;

    const int BOARD_SIZE = 15;
    const bool SIX_WIN = false;

    class Gomoku;

    template<>
    struct Move<Gomoku> {
        int row, col;
        bool operator==(Move<Gomoku> another) const { return row == another.row && col == another.col; }
    };

    class Gomoku : public IGame<Gomoku> {
        int _steps = 0;
        Color _color = Color::A;
        Status _status;
        Move<Gomoku> _last_move;

        class HalfBoard;
        void set_check_interval(int d, int& start, int& end);
        bool check_dir(const HalfBoard& hb, int dr, int dc);
        void check_status();

    public:
        class HalfBoard {
            bitset<256> _stones = 0;
            static int idx(uint8_t row, uint8_t col) {
                assert(row < BOARD_SIZE);
                assert(col < BOARD_SIZE);
                return (row << 4) | col;
            }
        public:
            bool operator==(const HalfBoard & bb) const { return (_stones ^ bb._stones).none(); }
            bool get(uint8_t row, uint8_t col) const { return _stones[idx(row, col)]; }
            bool get(Move<Gomoku> mov) const { return get(mov.row, mov.col); }
            bool safe_get(int8_t row, int8_t col) const {
                if (row < 0 | col < 0 | row >= BOARD_SIZE | col >= BOARD_SIZE) return false;
                return get(row, col);
            }
            bool safe_get(Move<Gomoku> mov) const {
                return safe_get(mov.row, mov.col);
            }
            void set(uint8_t row, uint8_t col, bool value) { _stones[idx(row, col)] = value; }
            void set(Move<Gomoku> mov, bool value) { set(mov.row, mov.col, value); }
            size_t get_hash() const {
                hash<decltype(_stones)> hash_fn;
                return hash_fn(_stones);
            }
            bool could_transfer_to(const HalfBoard & hb) const {
                bitset<256> res = _stones;
                res.flip();
                res |= hb._stones;
                return res.all();
            }
        } black, white;

        void move(Move<Gomoku> mov) override {
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

        Color get_color() const override { return _color; }
        Status get_status() const override { return _status; }
        size_t get_hash() const override { return hash<int>()(_steps) ^ black.get_hash() ^ white.get_hash(); }

        bool is_legal_move(Move<Gomoku> m) const override {
            assert(!_status.end);
            if (m.col < 0 || m.col >= BOARD_SIZE || m.row < 0 || m.row >= BOARD_SIZE) return false;
            return (!black.get(m) && !white.get(m));
        }

        vector<Move<Gomoku>> get_all_legal_moves() const override {
            vector<Move<Gomoku>> ms;
            for (int r = 0; r < BOARD_SIZE; r++) {
                for (int c = 0; c < BOARD_SIZE; c++) {
                    Move<Gomoku> m{ r,c };
                    if (is_legal_move(m)) ms.push_back(m);
                }
            }
            return ms;
        }

        bool could_transfer_to(const Gomoku & another) const override {
            if (_steps > another._steps) return false;
            return black.could_transfer_to(another.black) && white.could_transfer_to(another.white);
        }

        bool operator==(const Gomoku& another) const override { return _steps == another._steps && black == another.black && white == another.white; }

        string to_string() const {
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
    };
}