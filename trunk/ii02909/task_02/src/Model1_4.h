#pragma once
// ============================================================
// Модель 1.4: Дискретный интегратор с потерями (ЛР №2)
// Формула: y_{t+1} = a * y_t + b * (u_t - u_{t-1})
// ============================================================

#include "Model.h"
#include <cmath>
#include <sstream>

class Model1_4 : public Model {
private:
    // Константа для проверки устойчивости (|a| < 1)
    static constexpr double STABILITY_BOUND = 1.0;

    double a;
    double b;
    double y = 0.0;        // Текущее состояние
    double u_prev = 0.0;   // Предыдущий вход

public:
    Model1_4(double a_, double b_) : a(a_), b(b_) {}

    double next(double u) override {
        double y_next = a * y + b * (u - u_prev);
        u_prev = u;
        y = y_next;
        return y;
    }

    void reset() override {
        y = 0.0;
        u_prev = 0.0;
    }

    std::string name() const override {
        return "1.4: Discrete Integrator with Loss";
    }

    // Характеристическое уравнение: z - a = 0 (корень z = a).
    // Система устойчива, если |a| < 1.
    bool isStable() const override {
        return std::abs(a) < STABILITY_BOUND;
    }

    std::string stabilityWarning() const override {
        double abs_a = std::abs(a);
        if (abs_a < STABILITY_BOUND) {
            return "";
        }

        std::ostringstream oss;
        if (abs_a == STABILITY_BOUND) {
            // Граница устойчивости: колебания без затухания
            oss << "[WARN] Model 1.4: |a| = 1 (" << a
                << "), system is on the stability boundary (undamped oscillations).";
        }
        else {
            // Неустойчивая система: y будет расходиться
            oss << "[WARN] Model 1.4: |a| = " << abs_a
                << " >= 1, system is UNSTABLE, y will diverge!";
        }
        return oss.str();
    }
};