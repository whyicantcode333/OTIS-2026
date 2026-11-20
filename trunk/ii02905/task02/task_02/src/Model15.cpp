
#include "Model15.h"

Model15::Model15(double y0, double a1, double a2,
                 double b, int k, int n)
    : currentValue(y0),
      firstCoefficient(a1),
      delayedCoefficient(a2),
      inputCoefficient(b),
      delay(k),
      values(n + k + 2, y0)
{
}

double Model15::getDelayedValue(int tau) const
{
    if (tau < delay)
        return values.front();

    return values[tau - delay];
}

double Model15::step(double u, int tau)
{
    const double previousValue = currentValue;
    const double delayedValue = getDelayedValue(tau);

    currentValue =
        firstCoefficient * previousValue +
        delayedCoefficient * delayedValue +
        inputCoefficient * u;

    values[tau + 1] = currentValue;

    return currentValue;
}

double Model15::getY() const
{
    return currentValue;
}
