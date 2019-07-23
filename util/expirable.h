#pragma once

namespace Maestro {

    template<typename T>
    class ExpirableDefaultReset {
    public:
        T operator()() {
            return T();
        }
    };

    template<typename T, typename Reset = ExpirableDefaultReset<T>>
    class Expirable {
        int _timestamp = -1;
        T _value;
    public:
        T& value(int timestamp) {
            if (timestamp != _timestamp) _value = Reset()();
            _timestamp = timestamp;
            return _value;
        }
        T& operator()(int timestamp) {
            return value(timestamp);
        }
    };
}