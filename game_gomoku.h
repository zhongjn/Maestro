#pragma once
#include "game_base.h"
#include <cassert>
#include <bitset>

namespace Maestro {
    using namespace std;

    const int BOARD_SIZE = 15;
    const bool SIX_WIN = false;

    class Gomoku;

    template<>
    struct Move<Gomoku> {
        int row, col;
    };

    class Gomoku : public IGame<Gomoku> {

        Color _color = Color::A;
        Status _status;

        class HalfBoard;
        void set_check_interval(int d, int& start, int& end) {
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
        bool check_dir(const HalfBoard& hb, int dr, int dc) {
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
        void check_status() {
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
                    if (!black.get(r, c) || !white.get(r, c)) {
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

    public:
        class HalfBoard {
            std::bitset<256> _stones = 0;
            static int idx(uint8_t row, uint8_t col) {
                assert(row < BOARD_SIZE);
                assert(col < BOARD_SIZE);
                return (row << 4) | col;
            }
        public:
            bool operator==(const HalfBoard & bb) const { return _stones == bb._stones; }
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
        } black, white;

        void move(Move<Gomoku> mov) {
            assert(!_status.end);
            if (_color == Color::A) {
                assert(!black.get(mov));
                black.set(mov, true);
            }
            else if (_color == Color::B) {
                assert(!white.get(mov));
                white.set(mov, true);
            }
            _color = another_color(_color);

            check_status();
        }

        Color get_color() const { return _color; }
        Status get_status() const { return _status; }
        size_t get_hash() const { return black.get_hash() ^ white.get_hash(); }
        vector<Move<Gomoku>> get_all_legal_moves() const {
            vector<Move<Gomoku>> moves;
            for (int r = 0; r < BOARD_SIZE; r++) {
                for (int c = 0; c < BOARD_SIZE; c++) {
                    if (!black.get(r, c) && !white.get(r, c)) {
                        moves.push_back(Move<Gomoku>{r, c});
                    }
                }
            }
            return moves;
        }
        bool could_transfer_to(const Gomoku& another) const {
            return true;
        }
        bool operator==(const Gomoku& another) const {
            return black == another.black && white == another.white;
        }
    };
}