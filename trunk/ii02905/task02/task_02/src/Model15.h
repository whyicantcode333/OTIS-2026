
#pragma once

#include <vector>
#include "Model.h"

class Model15 : public Model
{
private:
    double currentValue;
    double firstCoefficient;
    double delayedCoefficient;
    double inputCoefficient;
    int delay;

    std::vector<double> values;

    double getDelayedValue(int tau) const;

public:
    Model15(double y0, double a1, double a2,
            double b, int k, int n);

    double step(double u, int tau) override;

    double getY() const override;
};
