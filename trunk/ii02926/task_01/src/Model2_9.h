#pragma once

#include "Model.h"
#include <cmath>

// Model 2.9: Square Root Modulated Action
// y_{t+1} = a * y_t + b * sqrt(|u_t|) * sign(u_t)
class Model2_9 : public Model {
private:
    double a;
    double b;
    double y = 0.0;

    static double sign(double x) {
        if (x > 0.0) return 1.0;
        if (x < 0.0) return -1.0;
        return 0.0;
    }

public:
    Model2_9(double a_, double b_)
        : a(a_), b(b_) {}

    double next(double u) override {
        const double u_mod = std::sqrt(std::abs(u)) * sign(u);
        y = a * y + b * u_mod;
        return y;
    }

    void reset() override {
        y = 0.0;
    }

    std::string name() const override {
        return "Model 2.9 (Square Root Modulated Action)";
    }

    bool isStable() const override { return true; }
    std::string stabilityWarning() const override { return ""; }
};
