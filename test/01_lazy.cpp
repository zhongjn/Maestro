#include "test.h"
#include "../util/lazy.h"

using namespace Maestro;

TEST_CASE(lazy_simple) {
    class Test {
        int _a;
    public:
        Test() = delete;
        Test(int a) : _a(a) {};
        int get_a() { return _a; }
    };

    int counter = 0;
    auto init = [&counter]() { counter++; return Test(2); };
    
    auto lazy = Lazy<Test>(init);
    expect(!lazy.initialized(), "shouldn't init yet");
    expect(counter == 0, "shouldn't init yet");
    int a = lazy->get_a();
    expect(lazy.initialized(), "should init yet");
    expect(counter == 1, "should init yet");
    expect(a == 2, "a value");
}