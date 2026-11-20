#include <gtest/gtest.h>
#include "Model3_1.h"
#include <cmath>

// ---------- Тест 1: значения по Эйлеру (a=1, dt=0.1) ----------
TEST(Model31Test, EulerStepValues) {
    Model3_1 model(1.0, 0.1);
    model.reset();

    // y_{k+1} = y_k * (1 - a*dt) = y_k * 0.9
    // Стартовое y0 = 1.0
    EXPECT_NEAR(model.next(0.0), 0.9, 1e-6);   // y1 = 1.0 * 0.9
    EXPECT_NEAR(model.next(0.0), 0.81, 1e-6);   // y2 = 0.9 * 0.9
    EXPECT_NEAR(model.next(0.0), 0.729, 1e-6);
    EXPECT_NEAR(model.next(0.0), 0.6561, 1e-6);
}

// ---------- Тест 2: сравнение с аналитикой y(t) = exp(-a*t) ----------
TEST(Model31Test, EulerApproximatesAnalyticalSolution) {
    const double a = 1.0;
    const double dt = 0.01;          // маленький шаг → точность выше
    const int steps = 100;           // t = 1.0

    Model3_1 model(a, dt);
    model.reset();

    double y = 1.0;
    for (int i = 0; i < steps; ++i) {
        y = model.next(0.0);
    }
    // Аналитика: y(1.0) = exp(-1) ~ 0.3679
    const double expected = std::exp(-a * dt * steps);
    EXPECT_NEAR(y, expected, 0.01);   // допуск 1% — метод Эйлера 1-го порядка
}

// ---------- Тест 3: reset() возвращает в y0=1 ----------
TEST(Model31Test, ResetRestoresInitialValue) {
    Model3_1 model(1.0, 0.1);
    model.reset();
    model.next(0.0);
    model.next(0.0);
    model.reset();
    // После reset первое значение = 0.9 (снова с 1.0)
    EXPECT_NEAR(model.next(0.0), 0.9, 1e-6);
}

// ---------- Тест 4: name() содержит "3.1" ----------
TEST(Model31Test, NameContainsModelIdentifier) {
    Model3_1 model(1.0, 0.1);
    EXPECT_NE(model.name().find("3.1"), std::string::npos);
}

// ============================================================
// Параметризованный тест
// ============================================================
class Model31ConvergenceTest : public ::testing::TestWithParam<double> {
protected:
    double runEuler(double dt) {
        Model3_1 model(1.0, dt);
        model.reset();
        const int steps = static_cast<int>(1.0 / dt);
        double y = 1.0;
        for (int i = 0; i < steps; ++i) {
            y = model.next(0.0);
        }
        return y;
    }
};

TEST_P(Model31ConvergenceTest, EulerConvergesToAnalytical) {
    const double dt = GetParam();
    const double y_euler = runEuler(dt);
    const double y_analytical = std::exp(-1.0);
    const double error = std::abs(y_euler - y_analytical);

    EXPECT_LT(error, dt * 10.0)
        << "dt = " << dt << ", error = " << error;
}

INSTANTIATE_TEST_SUITE_P(
    VariousDtValues,
    Model31ConvergenceTest,
    ::testing::Values(0.1, 0.05, 0.01, 0.001)
);