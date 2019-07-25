#pragma once
#include <functional>
#include "nullable.h"

namespace Maestro {
    using namespace std;

    template<typename T>
    class Lazy {
        function<T()> _init_fn;
        Nullable<T> _t;
    public:
        Lazy() = default;
        Lazy(function<T()> init_fn) noexcept : _init_fn(move(init_fn)) {}
        template<typename F>
        Lazy(F init_fn) : Lazy(function<T()>(init_fn)) {};
        bool initialized() const { return _t.has_v; }
        T& value() {
            if (!_t) {
                _t = _init_fn();
            }
            return _t.value();
        }
        T* operator->() {
            return &value();
        }
    };
}