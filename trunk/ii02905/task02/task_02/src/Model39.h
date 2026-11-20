
#pragma once

#include "Model.h"

class Model39 : public Model
{
private:
    double output;
    double integrationCoefficient;
    double timeStep;

    double calculateIncrement(double input) const;

public:
    Model39(double y0, double b, double dt);

    double step(double u, int tau) override;
    double getY() const override;
};
