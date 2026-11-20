
#include "Model27.h"

Model27::Model27(double y0, double a, double b, double epsilon)
    : output(y0),
      coefficientA(a),
      coefficientB(b),
      threshold(epsilon)
{
}

void Model27::updateRelayState(double input)
{
    if (input > threshold)
    {
        state = RelayState::Positive;
    }
    else if (input < -threshold)
    {
        state = RelayState::Negative;
    }
}

double Model27::getRelayOutput() const
{
    return static_cast<double>(state);
}

double Model27::step(double u, int)
{
    updateRelayState(u);

    const double relayValue = getRelayOutput();
    const double nextOutput =
        coefficientA * output + coefficientB * relayValue;

    output = nextOutput;

    return output;
}

double Model27::getY() const
{
    return output;
}
