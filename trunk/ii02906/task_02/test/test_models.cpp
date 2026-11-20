#include <gtest/gtest.h>
#include "LinearModel.h"
#include "NonLinearModel.h"
#include "DiffEqModel.h"
#include <cmath>

// ==========================================
// 1. БАЗОВЫЕ ТЕСТЫ
// ==========================================

TEST(LinearModelTest, StepCalculation) {
    LinearModel model(0.5, 0.0, 0.0, 1.0);
    EXPECT_DOUBLE_EQ(model.step(1.0), 1.0);
}

TEST(StabilityTest, StableCase) {
    LinearModel model(0.5, 0.2, 0.1, 1.0);
    EXPECT_TRUE(model.isStable());
}

TEST(StabilityTest, UnstableCase) {
    LinearModel model(2.0, 0.0, 0.0, 1.0);
    EXPECT_FALSE(model.isStable());
}

TEST(NonlinearModelTest, ZeroState) {
    NonLinearModel model(1.0, 1.0);
    EXPECT_DOUBLE_EQ(model.step(0.0), 0.0);
}

TEST(DiffEqModelTest, ZeroState) {
    DiffEqModel model(1.0, 0.01);
    EXPECT_DOUBLE_EQ(model.step(0.0), 0.0);
}

// ==========================================
// 2. ПАРАМЕТРИЗОВАННЫЕ ТЕСТЫ (Пункт 8 задания)
// ==========================================

// --- Тест для линейной модели ---
class LinearModelParamTest : public ::testing::TestWithParam<std::tuple<double, double, double, double>> {};

TEST_P(LinearModelParamTest, StepCalculationParams) {
    auto [a1, a2, a3, b] = GetParam();
    LinearModel model(a1, a2, a3, b);
    // При начальном состоянии 0 и входе 1, y_next = b * u
    EXPECT_DOUBLE_EQ(model.step(1.0), b);
}

INSTANTIATE_TEST_SUITE_P(
    LinearParams,
    LinearModelParamTest,
    ::testing::Values(
        std::make_tuple(0.5, 0.0, 0.0, 1.0), // Ожидаем 1.0
        std::make_tuple(0.1, 0.2, 0.3, 2.0), // Ожидаем 2.0
        std::make_tuple(0.0, 0.0, 0.0, 0.5)  // Ожидаем 0.5
    )
);

// --- Тест для нелинейной модели (Пункт 9 - Подмена сигналов) ---
class NonlinearModelParamTest : public ::testing::TestWithParam<double> {};

TEST_P(NonlinearModelParamTest, InputSignalTest) {
    NonLinearModel model(1.0, 1.0);
    double u = GetParam();
    // При y=0, tanh(0)=0, остается b*u^3 = u^3
    EXPECT_DOUBLE_EQ(model.step(u), std::pow(u, 3));
}

INSTANTIATE_TEST_SUITE_P(
    NonlinearParams,
    NonlinearModelParamTest,
    ::testing::Values(0.0, 1.0, 2.0) // Разные входные сигналы
);

// --- Тест для DiffEqModel (Диффур) ---
class DiffEqModelParamTest : public ::testing::TestWithParam<std::tuple<double, double>> {};

TEST_P(DiffEqModelParamTest, StepCalculationParams) {
    auto [k, dt] = GetParam();
    DiffEqModel model(k, dt);

    // Проверяем, что при нулевом входе и нулевом состоянии, 
    // состояние не меняется (производная равна 0)
    EXPECT_DOUBLE_EQ(model.step(0.0), 0.0);
}

INSTANTIATE_TEST_SUITE_P(
    DiffEqParams,
    DiffEqModelParamTest,
    ::testing::Values(
        std::make_tuple(1.0, 0.01),  // k=1, dt=0.01
        std::make_tuple(0.5, 0.1),   // k=0.5, dt=0.1
        std::make_tuple(2.0, 0.05)   // k=2, dt=0.05
    )
);