#include <iostream>
#include <cassert>

#include "../src/StabilityAnalyzer.h"

using namespace std;

void testStableModel()
{
    // All roots are inside the unit circle.
    bool result = StabilityAnalyzer::isStable(0.6, 0.2, 2);

    assert(result == true);

    cout << "Stable model: PASSED" << endl;
}

void testUnstableModel()
{
    // At least one root is outside the unit circle.
    bool result = StabilityAnalyzer::isStable(1.2, 0.2, 2);

    assert(result == false);

    cout << "Unstable model: PASSED" << endl;
}

void testInvalidDelay()
{
    bool result = StabilityAnalyzer::isStable(0.6, 0.2, 0);

    assert(result == false);

    cout << "Invalid delay: PASSED" << endl;
}

int main()
{
    cout << "=== STABILITY TESTS ===" << endl;

    testStableModel();
    testUnstableModel();
    testInvalidDelay();

    cout << "All stability tests passed!" << endl;

    return 0;
}