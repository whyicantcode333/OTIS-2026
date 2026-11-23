#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>
#include <tuple>

#include "Model1_9.h"
#include "Model2_6.h"
#include "Model3_9.h"
#include "InputSignal.h"

using ::testing::HasSubstr;

// --- Параметризованные тесты линейной модели 1.9 ---

struct Model1_9TestParam
{
    double a1;
    double a2;
    double a3;
    double b1;
    double y;
    double yPrev;
    double yPrev2;
    double u;
    double expected;
};

class Model1_9ParamTest : public ::testing::TestWithParam<Model1_9TestParam>
{
};

TEST_P(Model1_9ParamTest, CalculatesNextStateAccurately)
{
    const auto& p = GetParam();
    Model1_9 model(p.a1, p.a2, p.a3, p.b1);

    const double actual = model.calculateNext(p.y, p.yPrev, p.yPrev2, p.u, 0.1);
    EXPECT_DOUBLE_EQ(actual, p.expected);
}

INSTANTIATE_TEST_SUITE_P(
    LinearStepCases,
    Model1_9ParamTest,
    ::testing::Values(
        Model1_9TestParam{0.5, 0.2, 0.1, 0.4, 2.0, 1.0, 0.5, 1.0, 1.65},
        Model1_9TestParam{0.0, 0.0, 0.0, 1.5, 5.0, 4.0, 3.0, 2.0, 3.0},
        Model1_9TestParam{-0.3, 0.4, -0.2, 0.5, 1.0, 2.0, 3.0, -2.0, -1.1},
        Model1_9TestParam{0.8, 0.1, 0.05, 0.0, 0.0, 0.0, 0.0, 10.0, 0.0}
    )
);

// --- Проверка названий моделей через Google Mock matcher ---

TEST(ModelNamesTest, MatchExpectedSubstrings)
{
    Model1_9 m1(0.5, 0.2, 0.1, 0.4);
    Model2_6 m2(0.1, 0.2, 0.3);
    Model3_9 m3(1.0);

    EXPECT_THAT(m1.getName(), HasSubstr("Model 1.9"));
    EXPECT_THAT(m2.getName(), HasSubstr("Model 2.6"));
    EXPECT_THAT(m3.getName(), HasSubstr("Model 3.9"));
}

// --- Нелинейная модель 2.6 ---

TEST(Model2_6Test, ZeroStateAtZeroInputs)
{
    Model2_6 model(0.5, 0.3, 0.8);
    EXPECT_DOUBLE_EQ(model.calculateNext(0.0, 0.0, 0.0, 0.0, 0.1), 0.0);
}

TEST(Model2_6Test, QuadraticInputSymmetryAndCubicState)
{
    Model2_6 model(0.2, 0.5, 1.5);
    // y = 2.0, yPrev = 1.0, u = +-2.0 -> 0.2*8 - 0.5*1 + 1.5*4 = 1.6 - 0.5 + 6.0 = 7.1
    const double posInputResult = model.calculateNext(2.0, 1.0, 0.0, 2.0, 0.1);
    const double negInputResult = model.calculateNext(2.0, 1.0, 0.0, -2.0, 0.1);

    EXPECT_DOUBLE_EQ(posInputResult, 7.1);
    EXPECT_DOUBLE_EQ(posInputResult, negInputResult);
}

// --- Дифференциальная модель 3.9 ---

TEST(Model3_9Test, EulerStepWithHyperbolicTangent)
{
    Model3_9 model(2.0);
    const double y = 1.5;
    const double u = 0.8;
    const double dt = 0.25;

    const double expected = y + dt * 2.0 * std::tanh(u);
    EXPECT_DOUBLE_EQ(model.calculateNext(y, 0.0, 0.0, u, dt), expected);
}

TEST(Model3_9Test, StateUnchangedWhenTimeStepIsZero)
{
    Model3_9 model(5.0);
    EXPECT_DOUBLE_EQ(model.calculateNext(4.2, 1.0, 0.0, 3.0, 0.0), 4.2);
}

// --- Входные сигналы ---

TEST(InputSignalTest, StepSignalConstantAfterStart)
{
    EXPECT_DOUBLE_EQ(getInput(SignalType::Step, 1, 3.5), 3.5);
    EXPECT_DOUBLE_EQ(getInput(SignalType::Step, 15, 3.5), 3.5);
}

TEST(InputSignalTest, ImpulseSignalOnlyAtInitialMoment)
{
    const double u0 = getInput(SignalType::Impulse, 0, 4.0);
    const double u1 = getInput(SignalType::Impulse, 1, 4.0);
    const double u5 = getInput(SignalType::Impulse, 5, 4.0);

    // Один из моментов (t=0 или t=1) несет импульс амплитуды 4.0, далее 0.0
    EXPECT_DOUBLE_EQ(u0 + u1, 4.0);
    EXPECT_DOUBLE_EQ(u5, 0.0);
}

TEST(InputSignalTest, HarmonicSignalBoundedByAmplitude)
{
    const double amp = 2.5;
    for (int t = 0; t <= 10; ++t)
    {
        const double val = getInput(SignalType::Harmonic, t, amp);
        EXPECT_LE(std::abs(val), amp + 1e-12);
    }
}