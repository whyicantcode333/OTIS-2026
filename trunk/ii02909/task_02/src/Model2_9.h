#pragma once
// ============================================================
// Модель 2.9: Нелинейная модель с корневой модуляцией (ЛР №2)
// Формула: y_{t+1} = a * y_t + b * sqrt(|u_t|) * sign(u_t)
// ============================================================

#include "Model.h"
#include <cmath>

class Model2_9 : public Model {
private:
    double a;
    double b;
    double y = 0.0;

    // Вспомогательная функция: возвращает знак числа (-1, 0, 1)
    static double sign(double x) {
        if (x > 0.0) return 1.0;
        if (x < 0.0) return -1.0;
        return 0.0;
    }

public:
    Model2_9(double a_, double b_) : a(a_), b(b_) {}

    double next(double u) override {
        // Вычисляем нелинейное воздействие: корень из модуля с сохранением знака
        double u_modified = std::sqrt(std::abs(u)) * sign(u);
        y = a * y + b * u_modified;
        return y;
    }

    void reset() override {
        y = 0.0;
    }

    std::string name() const override {
        return "2.9: Square Root Modulated Action";
    }

    // Нелинейная система: линейный анализ (Z-плоскость) неприменим.
    // Устойчивость по умолчанию считается True.
    bool isStable() const override { return true; }
    std::string stabilityWarning() const override { return ""; }
};