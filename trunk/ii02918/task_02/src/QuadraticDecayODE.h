#pragma once
#include "DynamicModel.h"
#include "StabilityAnalyzer.h"
#include <cmath>
#include <sstream>

class QuadraticDecayODE : public DynamicModel
{
    static constexpr double INITIAL_Y = 1.0;

    double p1;
    double p2;
    double gain;
    double delta;
    double cur_y = INITIAL_Y;

public:
    QuadraticDecayODE(double a1, double a2, double b, double dt)
        : p1(a1), p2(a2), gain(b), delta(dt) {
    }

    double advance(double input) override
    {
        double deriv = -p1 * cur_y - p2 * cur_y * cur_y + gain * input;
        cur_y = cur_y + delta * deriv;
        return cur_y;
    }

    void reset() override { cur_y = INITIAL_Y; }

    std::string describe() const override
    {
        std::ostringstream out;
        out << "Model 3.8 (Quadratic Decay ODE, dt=" << delta << ")";
        return out.str();
    }

    bool verifyStability() const override
    {
        return StabilityAnalyzer::isStableEuler(p1, p2, delta, INITIAL_Y);
    }
};