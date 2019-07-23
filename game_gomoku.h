#pragma once
#include "game_base.h"
#include <cassert>
#include <bitset>

namespace Maestro {
    using namespace std;

    class Gomoku;

    template<>
    struct Move<Gomoku> {
        int row, col;
    };

    class Gomoku : public IGame<Gomoku> {
        Player _player;
        Status _status;
    public:
        class HalfBoard {
            std::bitset<256> _stones = 0;
            int idx(uint8_t row, uint8_t col) {
                assert(row < 15);
                assert(col < 15);
                return (row << 4) | col;
            }
        public:
            bool operator==(const HalfBoard& bb) const { return _stones == bb._stones; }
            bool get(uint8_t row, uint8_t col) { return _stones[idx(row, col)]; }
            bool get(Move<Gomoku> mov) { return get(mov.row, mov.col); }
            void set(uint8_t row, uint8_t col, bool value) { _stones[idx(row, col)] = value; }
            void set(Move<Gomoku> mov, bool value) { set(mov.row, mov.col, value); }
            size_t get_hash() const {
                hash<decltype(_stones)> hash_fn;
                return hash_fn(_stones);
            }
        } black, white;

        void move(Move<Gomoku> mov) {
            // TODO
            // 此处暂时假设A是黑，B是白
            // 在交换规则下，该假设不成立
            if (_player == Player::A) {
                assert(!black.get(mov));
                black.set(mov, true);
            }
            else if (_player == Player::B) {
                assert(!white.get(mov));
                white.set(mov, true);
            }
            _player = another_player(_player);

            // TODO: check status
        }

        Player get_player() const { return _player; }
        Status get_status() const { return _status; }
        size_t get_hash() const { return black.get_hash() ^ white.get_hash(); }
    };
}