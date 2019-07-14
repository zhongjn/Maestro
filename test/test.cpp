#include "test.h"

using namespace std;
vector<pair<string, void(*)()>> test_cases;

int main() {
    int cur = 1;
    int n_succeeded = 0;

    for (auto& p : test_cases) {
        stringstream ss_name;
        ss_name << "[" << cur << "/" << test_cases.size() << "] " << p.first;
        cout << ss_name.str() << endl;
        stringstream ss_status;

        void (*fp)() = p.second;
        try {
            fp();
            ss_status << "passed";
            n_succeeded++;
        }
        catch (AssertionError & ex) {
            ss_status << "assertion failed! " << ex.what();
        }
        catch (exception & ex) {
            ss_status << "unhandled error! " << ex.what();
        }

        cout << ss_name.str() << "- " << ss_status.str();
        cur++;
    }
    if (n_succeeded == test_cases.size()) {
        cout << "all tests passed" << endl;
    }
    return n_succeeded != test_cases.size();
}
