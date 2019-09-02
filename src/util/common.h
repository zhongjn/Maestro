#include <memory>

namespace Maestro {
    using namespace std;

    template<typename To, typename From>
    inline unique_ptr<To> cast_unique(unique_ptr<From>&& old) {
        return unique_ptr<To>{static_cast<To*>(old.release())};
    }
}