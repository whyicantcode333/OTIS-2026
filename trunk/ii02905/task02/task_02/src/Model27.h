
#pragma once

#include "Model.h"

class Model27 : public Model
{
private:
    enum class RelayState
    {
        Negative = -1,
        Neutral = 0,
        Positive = 1
    };

    double output;
    double coefficientA;
    double coefficientB;
    double threshold;

    RelayState state = RelayState::Neutral;

    void updateRelayState(double input);
    double getRelayOutput() const;

public:
    Model27(double y0, double a, double b, double epsilon);

    double step(double u, int tau) override;
    double getY() const override;
};
