#include "test.h"
#include "../util/expirable.h"

using namespace Maestro;

TEST_CASE(expirable) {
    Expirable<int> ex;
    ex(0) = 2;
    expect(ex(0) == 2, "value");
    expect(ex(1) == 0, "value");
    ex(1) += 3;
    ex(1) -= 6;
    expect(ex(1) == -3, "value");
    ex(2)++;
    expect(ex(2) == 1, "value");
}