#pragma once
#include <stdint.h>

namespace Maestro {
    class Random {
        static uint32_t _seed;
    public:
        // [0,max(uint)]
        static uint32_t value_i() {
            uint32_t x = _seed;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return _seed = x;
        }
        // [0,1]
        static float value_f() {
            return value_i() / float(UINT32_MAX);
        }
    };
}