#pragma once
#include "DynamicModel.h"
#include "StabilityAnalyzer.h"
#include <vector>
#include <cmath>
#include <sstream>

class StateDelayModel : public DynamicModel
{
    double f1;
    double f2;
    double g;
    int delay;
    std::vector<double> hist;

public:
    StateDelayModel(double a1, double a2, double b, int k)
        : f1(a1), f2(a2), g(b), delay(k)
    {
        reset();
    }

    double advance(double input) override
    {
        double y_old = hist.front();
        double y_now = hist.back();

        double y_next = f1 * y_now + f2 * y_old + g * input;

        hist.erase(hist.begin());
        hist.push_back(y_next);

        return y_next;
    }

    void reset() override
    {
        hist.assign(delay + 1, 0.0);
    }

    std::string describe() const override
    {
        std::ostringstream out;
        out << "Model 1.5 (State Delay, k=" << delay << ")";
        return out.str();
    }

    bool verifyStability() const override
    {
        return StabilityAnalyzer::isStableStateDelay(f1, f2);
    }
};