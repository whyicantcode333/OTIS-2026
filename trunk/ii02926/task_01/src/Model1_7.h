#pragma once

#include "Model.h"
#include <cmath>
#include <sstream>

// Model 1.7: System with Multi-Step Control History
// y_{t+1} = a * y_t + b1 * u_t + b2 * u_{t-1} + b3 * u_{t-2}
class Model1_7 : public Model {
private:
    static constexpr double STABILITY_BOUND = 1.0;

    double a;
    double b1;
    double b2;
    double b3;
    double y = 0.0;
    double u_prev1 = 0.0; // previous control input u_{t-1}
    double u_prev2 = 0.0; // previous control input u_{t-2}

public:
    Model1_7(double a_, double b1_, double b2_, double b3_)
        : a(a_), b1(b1_), b2(b2_), b3(b3_) {}

    double next(double u) override {
        const double y_next = a * y + b1 * u + b2 * u_prev1 + b3 * u_prev2;
        u_prev2 = u_prev1;
        u_prev1 = u;
        y = y_next;
        return y;
    }

    void reset() override {
        y = 0.0;
        u_prev1 = 0.0;
        u_prev2 = 0.0;
    }

    std::string name() const override {
        return "Model 1.7 (System with Multi-Step Control History)";
    }

    // Характеристическое уравнение однородной части: z - a = 0 => z = a.
    // Условие устойчивости на Z-плоскости: |a| < 1
    bool isStable() const override {
        return std::abs(a) < STABILITY_BOUND;
    }

    std::string stabilityWarning() const override {
        const double abs_a = std::abs(a);
        if (abs_a < STABILITY_BOUND) return "";

        std::ostringstream oss;
        if (abs_a == STABILITY_BOUND) {
            oss << "[WARN] Model 1.7: |a| = 1 (" << a
                << "), system is on stability boundary (undamped response).";
        } else {
            oss << "[WARN] Model 1.7: |a| = " << abs_a
                << " >= 1, system is UNSTABLE, output will diverge!";
        }
        return oss.str();
    }
};
