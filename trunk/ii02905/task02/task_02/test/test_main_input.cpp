#include <iostream>
#include <cassert>
#include <cmath>

#include "../src/InputSignal.h"

using namespace std;

bool approximatelyEqual(double a, double b)
{
    return abs(a - b) < 0.0001;
}

void testStepSignal()
{
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Step, 0), 1.0));
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Step, 5), 1.0));

    cout << "Step signal: PASSED" << endl;
}

void testImpulseSignal()
{
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Impulse, 0), 1.0));
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Impulse, 1), 0.0));
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Impulse, 5), 0.0));

    cout << "Impulse signal: PASSED" << endl;
}

void testHarmonicSignal()
{
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Harmonic, 0), 0.0));
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Harmonic, 1), sin(1.0)));
    assert(approximatelyEqual(InputSignal::getSignal(InputType::Harmonic, 2), sin(2.0)));

    cout << "Harmonic signal: PASSED" << endl;
}

int main()
{
    cout << "=== INPUT SIGNAL TESTS ===" << endl;

    testStepSignal();
    testImpulseSignal();
    testHarmonicSignal();

    cout << "All input signal tests passed!" << endl;

    return 0;
}