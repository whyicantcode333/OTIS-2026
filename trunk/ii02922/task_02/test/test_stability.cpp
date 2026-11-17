#include <gtest/gtest.h>
#include <vector>
#include <cmath>

#include "../src/stability.hpp"
#include "../src/models.hpp"

// ==========================================
// 1. ТЕСТЫ МОДУЛЯ АНАЛИЗА УСТОЙЧИВОСТИ
// ==========================================

TEST(StabilityModuleTest, StablePolynomial) {
    std::vector<double> stable_poly = { -0.5, 1.0 };
    EXPECT_TRUE(checkDiscreteStability(stable_poly));
}

TEST(StabilityModuleTest, UnstablePolynomial) {
    std::vector<double> unstable_poly = { -1.5, 1.0 };
    EXPECT_FALSE(checkDiscreteStability(unstable_poly));
}

// ==========================================
// 2. ПАРАМЕТРИЗОВАННЫЕ ТЕСТЫ (TEST_P) ДЛЯ МОДЕЛЕЙ
// Проверка расчета и сходимости при разных входных данных
// ==========================================

struct ModelTestParams {
    double initial_y;
    double input_u;
    double dt;
    double expected_y;
};

class Model1_10_ParamTest : public ::testing::TestWithParam<ModelTestParams> {};

TEST_P(Model1_10_ParamTest, ConvergenceCheck) {
    Model1_10 m1(0.5, 1.0, 0.1);
    ModelTestParams p = GetParam();

    double next_y = m1.calculateNext(p.initial_y, p.input_u, p.dt);
    EXPECT_NEAR(next_y, p.expected_y, 1e-5);
}

INSTANTIATE_TEST_CASE_P(
    Model1_10_Scenarios,
    Model1_10_ParamTest,
    ::testing::Values(
        ModelTestParams{ 1.0, 1.0, 0.1, 1.5 },   // Ступенчатое воздействие (u = 1.0)
        ModelTestParams{ 0.0, 0.0, 0.1, 0.0 },   // Нулевое воздействие
        ModelTestParams{ 2.0, 0.5, 0.1, 1.5 }    // Измененные начальные условия
    )
);

// ==========================================
// 3. ТЕСТИРОВАНИЕ РЕЖИМОВ УПРАВЛЕНИЯ (СИГНАЛОВ)
// ==========================================

TEST(InputSignalsTest, StepSignal) {
    EXPECT_DOUBLE_EQ(getInput(1, 0), 1.0);
    EXPECT_DOUBLE_EQ(getInput(1, 5), 1.0);
}

TEST(InputSignalsTest, ImpulseSignal) {
    EXPECT_DOUBLE_EQ(getInput(2, 0), 1.0);
    EXPECT_DOUBLE_EQ(getInput(2, 3), 0.0);
}

TEST(InputSignalsTest, HarmonicSignal) {
    int t = 2;
    EXPECT_DOUBLE_EQ(getInput(3, t), sin(t));
}