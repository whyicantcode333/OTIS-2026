#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>
#include <vector>
#include <cstdio>

#include "models.h"
#include "Model1_10.h"
#include "Model2_2.h"
#include "Model3_4.h"
#include "InputSignals.h"
#include "StabilityAnalyzer.h"
#include "Simulation.h"

using ::testing::Return;
using ::testing::_;

// ============================================================================
// 1. Тесты модуля анализа устойчивости (StabilityAnalyzerTest)
// ============================================================================
TEST(StabilityAnalyzerTest, StableCoefficientsReturnTrue) {
    EXPECT_TRUE(StabilityAnalyzer::isFirstOrderLinearStable(0.0));
    EXPECT_TRUE(StabilityAnalyzer::isFirstOrderLinearStable(0.5));
    EXPECT_TRUE(StabilityAnalyzer::isFirstOrderLinearStable(-0.99));

    Model1_10 model(0.7, 0.2, 0.1);
    EXPECT_TRUE(model.isStable());
    EXPECT_TRUE(model.stabilityWarning().empty());
}

TEST(StabilityAnalyzerTest, UnstableCoefficientsReturnFalseAndWarning) {
    EXPECT_FALSE(StabilityAnalyzer::isFirstOrderLinearStable(1.5));
    EXPECT_FALSE(StabilityAnalyzer::isFirstOrderLinearStable(-1.2));
    EXPECT_FALSE(StabilityAnalyzer::isFirstOrderLinearStable(NAN));

    Model1_10 model(1.25, 0.3, 0.1);
    EXPECT_FALSE(model.isStable());
    EXPECT_FALSE(model.stabilityWarning().empty());
}

TEST(StabilityAnalyzerTest, ZPlaneBoundaryValuesAreNotAsymptoticallyStable) {
    // Граница единичного круга |z| = 1
    EXPECT_FALSE(StabilityAnalyzer::isFirstOrderLinearStable(1.0));
    EXPECT_FALSE(StabilityAnalyzer::isFirstOrderLinearStable(-1.0));

    auto rootsPos = StabilityAnalyzer::getCharacteristicRoots(1.0);
    ASSERT_EQ(rootsPos.size(), 3u);
    EXPECT_DOUBLE_EQ(rootsPos[2], 1.0);

    auto rootsNeg = StabilityAnalyzer::getCharacteristicRoots(-1.0);
    ASSERT_EQ(rootsNeg.size(), 3u);
    EXPECT_DOUBLE_EQ(rootsNeg[2], -1.0);
}

// ============================================================================
// 2. Тесты на правильность выдаваемых значений — ручной расчёт (ManualCalculationTest)
// ============================================================================
TEST(ManualCalculationTest, Model1_10_ExactSteps) {
    // y[t+1] = 0.5*y[t] + 2.0*u[t] - 1.0*u[t-2]
    Model1_10 model(0.5, 2.0, -1.0);
    std::vector<double> u = { 1.0, 2.0, 3.0, 4.0 };
    double y0 = 0.0;

    auto y = runSimulation(model, u, y0, 0.1);
    ASSERT_EQ(y.size(), 4u);

    // t=0: y[1] = 0.5*0 + 2*1 - 1*0 = 2.0
    EXPECT_DOUBLE_EQ(y[1], 2.0);
    // t=1: y[2] = 0.5*2 + 2*2 - 1*0 = 5.0
    EXPECT_DOUBLE_EQ(y[2], 5.0);
    // t=2: y[3] = 0.5*5 + 2*3 - 1*1 = 2.5 + 6 - 1 = 7.5
    EXPECT_DOUBLE_EQ(y[3], 7.5);
}

TEST(ManualCalculationTest, Model2_2_ExactSteps) {
    // y[t+1] = 0.8*y[t] + 1.5*sat(u[t]), u in [-2.0, 2.0]
    Model2_2 model(0.8, 1.5, -2.0, 2.0);
    EXPECT_EQ(model.name(), "Nonlinear Model 2.2");

    // u = 5.0 -> насыщается до 2.0: y1 = 0.8*1.0 + 1.5*2.0 = 3.8
    EXPECT_DOUBLE_EQ(model.next(1.0, 5.0, 0.0, 0.1), 3.8);
    // u = -4.0 -> насыщается до -2.0: y2 = 0.8*3.8 + 1.5*(-2.0) = 0.04
    EXPECT_NEAR(model.next(3.8, -4.0, 0.0, 0.1), 0.04, 1e-12);
}

TEST(ManualCalculationTest, Model3_4_ExactSteps) {
    // y[t+1] = y[t] - dt * a * y[t]^2
    Model3_4 model(0.5);
    EXPECT_EQ(model.name(), "Differential Model 3.4");

    // y0 = 2.0, dt = 0.1 -> y1 = 2.0 - 0.1 * 0.5 * 4.0 = 1.8
    EXPECT_DOUBLE_EQ(model.next(2.0, 0.0, 0.0, 0.1), 1.8);
}

// ============================================================================
// 3. Граничные значения нелинейностей (BoundaryValueTest)
// ============================================================================
TEST(BoundaryValueTest, Model2_2_SaturationThresholds) {
    const double uMin = -3.0;
    const double uMax = 4.0;
    Model2_2 model(0.0, 1.0, uMin, uMax);

    // Точно на границах u = uMin и u = uMax
    EXPECT_DOUBLE_EQ(model.next(0.0, uMin, 0.0, 0.1), uMin);
    EXPECT_DOUBLE_EQ(model.next(0.0, uMax, 0.0, 0.1), uMax);

    // Непосредственно за границами
    EXPECT_DOUBLE_EQ(model.next(0.0, uMin - 0.001, 0.0, 0.1), uMin);
    EXPECT_DOUBLE_EQ(model.next(0.0, uMax + 0.001, 0.0, 0.1), uMax);

    // Внутри линейной зоны
    EXPECT_DOUBLE_EQ(model.next(0.0, 1.25, 0.0, 0.1), 1.25);
}

// ============================================================================
// 4. Тесты инвариантов и физического смысла (PhysicsAndInvariantsTest)
// ============================================================================
TEST(PhysicsAndInvariantsTest, ZeroInputAndZeroStateRemainAtRest) {
    Model1_10 m1(0.9, 0.5, 0.2);
    Model2_2 m2(0.9, 0.5, -1.0, 1.0);
    Model3_4 m3(0.5);

    std::vector<double> zeroInput(20, 0.0);

    for (double val : runSimulation(m1, zeroInput, 0.0, 0.1)) {
        EXPECT_DOUBLE_EQ(val, 0.0);
    }
    for (double val : runSimulation(m2, zeroInput, 0.0, 0.1)) {
        EXPECT_DOUBLE_EQ(val, 0.0);
    }
    for (double val : runSimulation(m3, zeroInput, 0.0, 0.1)) {
        EXPECT_DOUBLE_EQ(val, 0.0);
    }
}

TEST(PhysicsAndInvariantsTest, Model3_4_EulerMatchesAnalyticalSolution) {
    // ОДУ: dy/dt = -a * y^2, y(0) = y0
    // Аналитическое решение: y(t) = y0 / (1 + a * y0 * t)
    const double a = 0.5;
    const double y0 = 2.0;
    const double dt = 0.001;
    const std::size_t steps = 1000; // t = 1.0 с

    Model3_4 model(a);
    std::vector<double> u(steps + 1, 0.0);
    auto y = runSimulation(model, u, y0, dt);

    double tEnd = static_cast<double>(steps) * dt;
    double analyticalY = y0 / (1.0 + a * y0 * tEnd); // 2 / (1 + 1) = 1.0

    EXPECT_NEAR(y.back(), analyticalY, 1e-3);
}

// ============================================================================
// 5. Параметризованные тесты сходимости (SteadyStateCases/LinearConvergenceTest)
// ============================================================================
struct ConvergenceParam {
    double a1;
    double b1;
    double b2;
    double uConst;
    double expectedSteadyState;
};

class LinearConvergenceTest : public ::testing::TestWithParam<ConvergenceParam> {};

TEST_P(LinearConvergenceTest, ConvergesToTheoreticalSteadyState) {
    const auto& p = GetParam();
    Model1_10 model(p.a1, p.b1, p.b2);
    ASSERT_TRUE(model.isStable());

    std::vector<double> u(200, p.uConst);
    auto y = runSimulation(model, u, 0.0, 0.1);

    // Установившееся значение: y_inf = (b1 + b2) / (1 - a1) * u
    EXPECT_NEAR(y.back(), p.expectedSteadyState, 1e-5);
}

INSTANTIATE_TEST_SUITE_P(
    SteadyStateCases,
    LinearConvergenceTest,
    ::testing::Values(
        ConvergenceParam{ 0.5, 1.0, 0.5, 2.0, 6.0 },   // (1.5 / 0.5) * 2 = 6.0
        ConvergenceParam{ 0.2, 0.4, 0.4, 1.0, 1.0 },   // (0.8 / 0.8) * 1 = 1.0
        ConvergenceParam{ -0.5, 1.5, 0.0, 3.0, 3.0 }   // (1.5 / 1.5) * 3 = 3.0
    )
);

// ============================================================================
// 6. Тестирование режимов управления через Google Mock (ControlModesMockTest)
// ============================================================================
class MockInputSignalProvider : public IInputSignalProvider {
public:
    MOCK_METHOD(double, getSignal, (std::size_t step, double time), (const, override));
};

TEST(ControlModesMockTest, StepControlModeViaMock) {
    MockInputSignalProvider mockProvider;
    EXPECT_CALL(mockProvider, getSignal(_, _))
        .Times(4)
        .WillRepeatedly(Return(2.0));

    Model1_10 model(0.5, 1.0, 0.0);
    auto y = runSimulationWithProvider(model, mockProvider, 3, 0.0, 0.1);

    ASSERT_EQ(y.size(), 4u);
    EXPECT_DOUBLE_EQ(y[1], 2.0);
    EXPECT_DOUBLE_EQ(y[2], 3.0);
    EXPECT_DOUBLE_EQ(y[3], 3.5);
}

TEST(ControlModesMockTest, ImpulseControlModeViaMock) {
    MockInputSignalProvider mockProvider;
    EXPECT_CALL(mockProvider, getSignal(0, _)).WillOnce(Return(4.0));
    EXPECT_CALL(mockProvider, getSignal(1, _)).WillOnce(Return(0.0));
    EXPECT_CALL(mockProvider, getSignal(2, _)).WillOnce(Return(0.0));
    EXPECT_CALL(mockProvider, getSignal(3, _)).WillOnce(Return(0.0));

    Model1_10 model(0.5, 1.0, 0.0);
    auto y = runSimulationWithProvider(model, mockProvider, 3, 0.0, 0.1);

    EXPECT_DOUBLE_EQ(y[1], 4.0);
    EXPECT_DOUBLE_EQ(y[2], 2.0);
    EXPECT_DOUBLE_EQ(y[3], 1.0);
}

TEST(ControlModesMockTest, HarmonicControlModeViaMock) {
    MockInputSignalProvider mockProvider;
    EXPECT_CALL(mockProvider, getSignal(0, _)).WillOnce(Return(0.0));
    EXPECT_CALL(mockProvider, getSignal(1, _)).WillOnce(Return(1.0));
    EXPECT_CALL(mockProvider, getSignal(2, _)).WillOnce(Return(0.0));
    EXPECT_CALL(mockProvider, getSignal(3, _)).WillOnce(Return(-1.0));

    Model1_10 model(0.0, 2.0, 0.0);
    auto y = runSimulationWithProvider(model, mockProvider, 3, 0.0, 0.1);

    EXPECT_DOUBLE_EQ(y[1], 0.0);
    EXPECT_DOUBLE_EQ(y[2], 2.0);
    EXPECT_DOUBLE_EQ(y[3], 0.0);
}

// ============================================================================
// 7. Тесты стандартных генераторов сигналов и экспорта (StandardSignalsAndExportTest)
// ============================================================================
TEST(StandardSignalsAndExportTest, SignalGeneratorsAndHtmlReport) {
    EXPECT_DOUBLE_EQ(generateInput(InputSignalType::Constant, 5, 0.5, 3.0, 1.0), 3.0);
    EXPECT_DOUBLE_EQ(generateInput(InputSignalType::Impulse, 0, 0.0, 3.0, 1.0), 3.0);
    EXPECT_DOUBLE_EQ(generateInput(InputSignalType::Impulse, 1, 0.1, 3.0, 1.0), 0.0);
    EXPECT_NEAR(generateInput(InputSignalType::Harmonic, 1, 0.5, 2.0, 1.0), 2.0 * std::sin(0.5), 1e-12);

    StandardInputSignalProvider stdProv(InputSignalType::Constant, 1.0, 1.0);
    EXPECT_DOUBLE_EQ(stdProv.getSignal(0, 0.0), 1.0);

    Model1_10 model(0.5, 1.0, 0.0);
    std::vector<double> emptyU;
    EXPECT_TRUE(runSimulation(model, emptyU, 0.0, 0.1).empty());

    const std::string tmpHtml = "test_temp_report.html";
    std::vector<double> u = { 1.0, 1.0, 1.0 };
    auto y = runSimulation(model, u, 0.0, 0.1);
    EXPECT_TRUE(exportSimulationToHtml(tmpHtml, model, u, y, 0.1));
    std::remove(tmpHtml.c_str());
}