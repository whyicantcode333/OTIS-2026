#pragma once
// ============================================================
// Модель 3.1: Чистое линейное затухание (ЛР №2)
// Дифференциальное уравнение: dy/dt = -a * y
// Решается методом Эйлера: y_{t+1} = y_t + dt * (-a * y_t)
// ============================================================

#include "Model.h"
#include <cmath>

class Model3_1 : public Model {
private:
    static constexpr double DEFAULT_DT = 0.01;
    static constexpr double INITIAL_Y = 1.0;   // Начальное условие y(0) = 1
    static constexpr double STABILITY_BOUND = 1.0;

    double a;
    double dt;
    double y = INITIAL_Y;

public:
    explicit Model3_1(double a_, double dt_ = DEFAULT_DT) : a(a_), dt(dt_) {}

    double next(double /*u*/) override {
        // Входной сигнал u здесь не используется
        double dy = -a * y;
        y = y + dt * dy; // Шаг интегрирования по Эйлеру
        return y;
    }

    void reset() override {
        y = INITIAL_Y;
    }

    std::string name() const override {
        return "3.1: Pure Linear Decay";
    }

    // Условие устойчивости численной схемы: |1 - a*dt| < 1
    bool isStable() const override {
        return std::abs(1.0 - a * dt) < STABILITY_BOUND;
    }

    std::string stabilityWarning() const override {
        // Проверка: если шаг dt слишком велик, метод Эйлера может разойтись
        if (double z = 1.0 - a * dt; std::abs(z) < STABILITY_BOUND) {
            return "";
        }
        return "[WARN] Model 3.1: |1 - a*dt| >= 1, Euler scheme is numerically unstable. Decrease dt.";
    }
};