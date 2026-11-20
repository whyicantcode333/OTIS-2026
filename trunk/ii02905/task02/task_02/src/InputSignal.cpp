#include "InputSignal.h"
#include <cmath>

double InputSignal::getSignal(InputType type, int tau)
{
    switch (type)
    {
    case InputType::Step:
        return 1.0;

    case InputType::Impulse:
        return (tau == 0) ? 1.0 : 0.0;

    case InputType::Harmonic:
        return std::sin(static_cast<double>(tau));

    default:
        return 0.0;
    }
}