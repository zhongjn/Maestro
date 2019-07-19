#pragma once
#include <type_traits>
#include <functional>

namespace Maestro {
    using namespace std;

    template<typename T>
    class Lazy {
        function<T()> _init_fn;
        bool _init = false;
        union { T _value; };
    public:
        Lazy() = delete;
        Lazy(const Lazy&) = delete;
        Lazy(Lazy&&) = delete;
        Lazy(function<T()> init_fn) noexcept : _init_fn(move(init_fn)) {}
        ~Lazy() {
            if (_init) {
                _value.~T();
            }
        }
        T& value() {
            if (!_init) {
                new(&_value) T(_init_fn());
            }
            return _value;
        }
        T* operator->() {
            return &value();
        }
    };
}