
#include "Model39.h"
#include <cmath>

Model39::Model39(double y0, double b, double dt)
    : output(y0),
      integrationCoefficient(b),
      timeStep(dt)
{
}

double Model39::calculateIncrement(double input) const
{
    const double transformedInput = std::tanh(input);

    return timeStep * integrationCoefficient * transformedInput;
}

double Model39::step(double u, int)
{
    const double increment = calculateIncrement(u);

    output += increment;

    return output;
}

double Model39::getY() const
{
    return output;
}
