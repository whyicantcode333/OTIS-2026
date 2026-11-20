#include <iostream>
#include <cassert>
#include <cmath>

#include "../src/Model15.h"
#include "../src/Model27.h"
#include "../src/Model39.h"

using namespace std;

bool approximatelyEqual(double a, double b)
{
    return abs(a - b) < 0.0001;
}

void testModel15()
{
    Model15 model(0.0, 0.6, 0.2, 0.5, 2, 10);

    double y1 = model.step(1.0, 0);
    double y2 = model.step(1.0, 1);
    double y3 = model.step(1.0, 2);

    assert(approximatelyEqual(y1, 0.5));
    assert(approximatelyEqual(y2, 0.8));
    assert(approximatelyEqual(y3, 0.98));

    cout << "Model 1.5: PASSED" << endl;
}

void testModel27()
{
    Model27 model(0.0, 0.8, 0.7, 0.2);

    double y1 = model.step(1.0, 0);
    double y2 = model.step(1.0, 1);
    double y3 = model.step(0.0, 2);

    assert(approximatelyEqual(y1, 0.7));
    assert(approximatelyEqual(y2, 1.26));
    assert(approximatelyEqual(y3, 1.708));

    cout << "Model 2.7: PASSED" << endl;
}

void testModel39()
{
    Model39 model(0.0, 1.0, 0.1);

    double y = model.step(1.0, 0);

    double expected = 0.1 * tanh(1.0);

    assert(approximatelyEqual(y, expected));

    cout << "Model 3.9: PASSED" << endl;
}

int main()
{
    cout << "=== MODEL TESTS ===" << endl;

    testModel15();
    testModel27();
    testModel39();

    cout << "All model tests passed!" << endl;

    return 0;
}