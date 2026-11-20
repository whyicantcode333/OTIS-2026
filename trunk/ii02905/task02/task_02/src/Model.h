
#pragma once

class Model
{
public:
    virtual ~Model() = default;

    virtual double step(double u, int tau) = 0;

    virtual double getY() const = 0;
};
