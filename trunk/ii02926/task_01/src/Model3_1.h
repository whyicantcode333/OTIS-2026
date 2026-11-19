#pragma once

#include "Model.h"
#include <cmath>

// Model 3.1: Pure Linear Decay
// dy/dt = -a * y => y_{t+1} = y_t + dt * (-a * y_t)
class Model3_1 : public Model {
private:
    static constexpr double DEFAULT_DT = 0.01;
    static constexpr double INITIAL_Y = 1.0;
    static constexpr double STABILITY_BOUND = 1.0;

    double a;
    double dt;
    double y = INITIAL_Y;

public:
    explicit Model3_1(double a_, double dt_ = DEFAULT_DT)
        : a(a_), dt(dt_) {}

    double next(double /*u*/) override {
        const double dy = -a * y;
        y = y + dt * dy;
        return y;
    }

    void reset() override {
        y = INITIAL_Y;
    }

    std::string name() const override {
        return "Model 3.1 (Pure Linear Decay)";
    }

    // Устойчивость схемы Эйлера: |1 - a*dt| < 1
    bool isStable() const override {
        return std::abs(1.0 - a * dt) < STABILITY_BOUND;
    }

    std::string stabilityWarning() const override {
        if (const double z = 1.0 - a * dt; std::abs(z) < STABILITY_BOUND) {
            return "";
        }
        return "[WARN] Model 3.1: |1 - a*dt| >= 1, Euler scheme is numerically unstable. Decrease dt.";
    }
};
