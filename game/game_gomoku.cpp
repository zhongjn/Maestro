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