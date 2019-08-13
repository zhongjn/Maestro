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
        int _steps = 0;
        Color _color = Color::A;
        Status _status;

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
            bool could_transfer_to(const HalfBoard & hb) const {
                bitset<256> res = _stones;
                res.flip();
                res |= hb._stones;
                return res.any();
            }
        } black, white;

        void move(Move<Gomoku> mov) override {
            _steps++;
            assert(!_status.end);
            assert(is_legal_move(mov));
            assert(!black.get(mov) && !white.get(mov));
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

        bool operator==(const Gomoku & another) const override { return _steps == another._steps && black == another.black && white == another.white; }
    };
}