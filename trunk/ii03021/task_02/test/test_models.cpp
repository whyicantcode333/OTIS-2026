// Unit tests of the three models of variant 2:
//   1.2  y[t+1] = a*y[t] + b*u[t-k]
//   2.4  y[t+1] = a*y[t]*cos(y[t]) + b*ln(1+|u[t]|)
//   3.6  dy/dt = a*y^3 + b*u (explicit Euler)
// Expected values were calculated by hand (see comments) and are written as literals.
#include <gtest/gtest.h>

#include "models.hpp"
#include "signals.hpp"
#include "stability.hpp"

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using otis::CubicDifferentialModel;
using otis::DelayedControlModel;
using otis::LogarithmicInteractionModel;
using otis::Model;
using otis::SignalType;

namespace {

std::vector<double> run(Model& model, SignalType signal, int steps) {
    model.reset();
    std::vector<double> y;
    for (int tau = 0; tau < steps; ++tau) {
        y.push_back(model.step(otis::inputSignal(signal, tau), tau));
    }
    return y;
}

std::string labelOf(const char* label) { return std::string(label); }

// ---------------------------------------------------------------- TEST_P: hand-calculated steps

struct DelayedCase {
    const char* label;
    double a, b;
    int k;
    SignalType signal;
    std::vector<double> expected;  // y(1), y(2), ... as returned by step(tau = 0, 1, ...)
};

class DelayedStepTest : public ::testing::TestWithParam<DelayedCase> {};

TEST_P(DelayedStepTest, MatchesHandCalculatedValues) {
    const DelayedCase& c = GetParam();
    DelayedControlModel model(c.a, c.b, c.k);
    const auto y = run(model, c.signal, static_cast<int>(c.expected.size()));
    ASSERT_EQ(y.size(), c.expected.size());
    for (std::size_t i = 0; i < y.size(); ++i) {
        EXPECT_NEAR(y[i], c.expected[i], 1e-9) << c.label << ", tau=" << i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Model12, DelayedStepTest,
    ::testing::Values(
        // a=0.5 b=2 k=1, u=1: 0; 2; 0.5*2+2=3; 0.5*3+2=3.5; 3.75; 3.875
        DelayedCase{"step_a05_b2_k1", 0.5, 2.0, 1, SignalType::Step,
                    {0.0, 2.0, 3.0, 3.5, 3.75, 3.875}},
        // a=0.5 b=1 k=2, impulse: u0=1 reaches y at tau=2; then geometric decay by a
        DelayedCase{"impulse_a05_b1_k2", 0.5, 1.0, 2, SignalType::Impulse,
                    {0.0, 0.0, 1.0, 0.5, 0.25, 0.125, 0.0625}},
        // a=0 -> y(tau+1) = b*u(tau-k) = sin(tau-1): 0, sin0, sin1, sin2, sin3
        DelayedCase{"harmonic_a0_b1_k1", 0.0, 1.0, 1, SignalType::Harmonic,
                    {0.0, 0.0, 0.8414709848, 0.9092974268, 0.1411200081}},
        // a=-0.5 b=1 k=3: first non-zero value at tau=4 (= sin 1), then y = -0.5*y + sin(tau-3)
        DelayedCase{"harmonic_aneg05_b1_k3", -0.5, 1.0, 3, SignalType::Harmonic,
                    {0.0, 0.0, 0.0, 0.0, 0.8414709848, 0.4885619344, -0.1031609592}},
        // b = 0 -> output is identically zero
        DelayedCase{"step_b0", 0.9, 0.0, 1, SignalType::Step, {0.0, 0.0, 0.0, 0.0}}),
    [](const ::testing::TestParamInfo<DelayedCase>& info) { return labelOf(info.param.label); });

struct LogCase {
    const char* label;
    double a, b;
    SignalType signal;
    std::vector<double> expected;
};

class LogStepTest : public ::testing::TestWithParam<LogCase> {};

TEST_P(LogStepTest, MatchesHandCalculatedValues) {
    const LogCase& c = GetParam();
    LogarithmicInteractionModel model(c.a, c.b);
    const auto y = run(model, c.signal, static_cast<int>(c.expected.size()));
    ASSERT_EQ(y.size(), c.expected.size());
    for (std::size_t i = 0; i < y.size(); ++i) {
        EXPECT_NEAR(y[i], c.expected[i], 1e-9) << c.label << ", tau=" << i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Model24, LogStepTest,
    ::testing::Values(
        // y1 = ln 2 = 0.6931471806; y2 = 0.5*y1*cos(y1) + ln 2 = 0.9597450684 ...
        LogCase{"step_a05_b1", 0.5, 1.0, SignalType::Step,
                {0.6931471806, 0.9597450684, 0.9684638763, 0.9674965281, 0.9676079674}},
        // a=0: y = 2*ln(1+|sin tau|)
        LogCase{"harmonic_a0_b2", 0.0, 2.0, SignalType::Harmonic,
                {0.0, 1.2211294010, 1.2934706701, 0.2640204866, 1.1269907854}},
        // impulse: ln2 once, then y = 0.5*y*cos(y)
        LogCase{"impulse_a05_b1", 0.5, 1.0, SignalType::Impulse,
                {0.6931471806, 0.2665978878, 0.1285898611, 0.0637640930, 0.0318172545}},
        LogCase{"step_aneg05_b1", -0.5, 1.0, SignalType::Step,
                {0.6931471806, 0.4265492927, 0.4989821911, 0.4740765166, 0.4822507608}}),
    [](const ::testing::TestParamInfo<LogCase>& info) { return labelOf(info.param.label); });

struct EulerCase {
    const char* label;
    double a, b, dt;
    SignalType signal;
    std::vector<double> expected;
};

class EulerStepTest : public ::testing::TestWithParam<EulerCase> {};

TEST_P(EulerStepTest, MatchesHandCalculatedValues) {
    const EulerCase& c = GetParam();
    CubicDifferentialModel model(c.a, c.b, c.dt);
    const auto y = run(model, c.signal, static_cast<int>(c.expected.size()));
    ASSERT_EQ(y.size(), c.expected.size());
    for (std::size_t i = 0; i < y.size(); ++i) {
        EXPECT_NEAR(y[i], c.expected[i], 1e-9) << c.label << ", tau=" << i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    Model36, EulerStepTest,
    ::testing::Values(
        // y1 = 0.1*(0+1) = 0.1; y2 = 0.1 + 0.1*(-0.001+1) = 0.1999; y3 = 0.2991011994 ...
        EulerCase{"step_aneg1_b1_dt01", -1.0, 1.0, 0.1, SignalType::Step,
                  {0.1, 0.1999, 0.2991011994, 0.3964253944, 0.4901954467}},
        // a=0: y(n+1) = y(n) + dt*b*sin(n) = 0, 1*sin1, ...
        EulerCase{"harmonic_a0_b2_dt05", 0.0, 2.0, 0.5, SignalType::Harmonic,
                  {0.0, 0.8414709848, 1.7507684116, 1.8918884197}},
        EulerCase{"impulse_a1_b1_dt01", 1.0, 1.0, 0.1, SignalType::Impulse,
                  {0.1, 0.1001, 0.1002003003, 0.1003009024}}),
    [](const ::testing::TestParamInfo<EulerCase>& info) { return labelOf(info.param.label); });

// ---------------------------------------------------------------- TEST_P: convergence

struct ConvCase {
    double a, b;
    int k;
};

class DelayedConvergenceTest : public ::testing::TestWithParam<ConvCase> {};

// Step response of a stable model converges to y* = b / (1 - a) and the analyzer says "stable".
TEST_P(DelayedConvergenceTest, StepResponseConvergesToSteadyState) {
    const ConvCase c = GetParam();
    ASSERT_TRUE(otis::analyzeDelayedControl(c.a, c.k).isStable());
    DelayedControlModel model(c.a, c.b, c.k);
    const auto y = run(model, SignalType::Step, 400);
    EXPECT_NEAR(y.back(), c.b / (1.0 - c.a), 1e-6);
    EXPECT_NEAR(y[398], y[399], 1e-6);
}

INSTANTIATE_TEST_SUITE_P(
    StableCoefficients, DelayedConvergenceTest,
    ::testing::Values(ConvCase{0.5, 1.0, 1}, ConvCase{-0.5, 2.0, 3}, ConvCase{0.9, 0.1, 2},
                      ConvCase{0.0, 1.0, 1}, ConvCase{-0.9, 1.0, 5}));

class DelayedDivergenceTest : public ::testing::TestWithParam<double> {};

TEST_P(DelayedDivergenceTest, UnstableCoefficientDiverges) {
    const double a = GetParam();
    ASSERT_EQ(otis::analyzeDelayedControl(a, 1).status, otis::Stability::Unstable);
    DelayedControlModel model(a, 1.0, 1);
    const auto y = run(model, SignalType::Step, 100);
    EXPECT_GT(std::abs(y.back()), 1e3) << "a=" << a;
}

INSTANTIATE_TEST_SUITE_P(UnstableCoefficients, DelayedDivergenceTest,
                         ::testing::Values(1.1, -1.1, 2.0, -2.0));

// After the impulse has passed, y(t+1)/y(t) = a, i.e. the numerical dynamics follow the Z-plane root.
class DelayedImpulseRatioTest : public ::testing::TestWithParam<double> {};

TEST_P(DelayedImpulseRatioTest, SuccessiveOutputsRatioEqualsRoot) {
    const double a = GetParam();
    DelayedControlModel model(a, 1.0, 2);
    const auto y = run(model, SignalType::Impulse, 8);
    EXPECT_DOUBLE_EQ(y[2], 1.0);  // b * u0 arrives at tau = k
    for (std::size_t n = 2; n + 1 < y.size(); ++n) {
        EXPECT_NEAR(y[n + 1] / y[n], a, 1e-12) << "n=" << n;
    }
    EXPECT_NEAR(otis::analyzeDelayedControl(a, 2).spectralRadius, std::abs(a), 1e-12);
}

INSTANTIATE_TEST_SUITE_P(Roots, DelayedImpulseRatioTest,
                         ::testing::Values(-1.5, -0.8, 0.3, 0.9, 1.2));

struct LogConvCase {
    double a, b;
};

class LogConvergenceTest : public ::testing::TestWithParam<LogConvCase> {};

// Step response of the stable nonlinear model settles at a fixed point y* = a*y*cos(y*) + b*ln 2.
TEST_P(LogConvergenceTest, StepResponseSettlesAtFixedPoint) {
    const LogConvCase c = GetParam();
    ASSERT_TRUE(otis::analyzeLogarithmic(c.a).isStable());
    LogarithmicInteractionModel model(c.a, c.b);
    const auto y = run(model, SignalType::Step, 300);
    EXPECT_NEAR(y[298], y[299], 1e-9);
    EXPECT_NEAR(y.back(), c.a * y.back() * std::cos(y.back()) + c.b * std::log(2.0), 1e-9);
}

INSTANTIATE_TEST_SUITE_P(StableCoefficients, LogConvergenceTest,
                         ::testing::Values(LogConvCase{0.5, 1.0}, LogConvCase{-0.5, 1.0},
                                           LogConvCase{0.2, 0.5}));

// ---------------------------------------------------------------- TEST_P: invariants

struct FactoryCase {
    const char* label;
    std::function<std::unique_ptr<Model>()> make;
};

class ModelInvariantTest : public ::testing::TestWithParam<FactoryCase> {};

// u = 0 and y = 0  ->  the system stays at rest.
TEST_P(ModelInvariantTest, ZeroInputAndZeroStateStayAtRest) {
    auto model = GetParam().make();
    for (int tau = 0; tau < 50; ++tau) {
        EXPECT_EQ(model->step(0.0, tau), 0.0) << "tau=" << tau;
    }
}

TEST_P(ModelInvariantTest, ResetRestoresInitialState) {
    auto model = GetParam().make();
    const auto first = run(*model, SignalType::Harmonic, 20);
    const auto second = run(*model, SignalType::Harmonic, 20);  // run() calls reset() first
    EXPECT_EQ(first, second);
    model->reset();
    EXPECT_EQ(model->step(0.0, 0), 0.0);
}

TEST_P(ModelInvariantTest, NameIsNotEmpty) {
    auto model = GetParam().make();
    ASSERT_NE(model->name(), nullptr);
    EXPECT_GT(std::string(model->name()).size(), 0u);
}

INSTANTIATE_TEST_SUITE_P(
    AllModels, ModelInvariantTest,
    ::testing::Values(
        FactoryCase{"Delayed", [] { return std::make_unique<DelayedControlModel>(0.5, 1.0, 2); }},
        FactoryCase{"Logarithmic",
                    [] { return std::make_unique<LogarithmicInteractionModel>(0.5, 1.0); }},
        FactoryCase{"Euler",
                    [] { return std::make_unique<CubicDifferentialModel>(-1.0, 1.0, 0.1); }}),
    [](const ::testing::TestParamInfo<FactoryCase>& info) { return labelOf(info.param.label); });

} // namespace

// ---------------------------------------------------------------- plain tests

TEST(ModelNames, ReturnExpectedTitles) {
    EXPECT_STREQ(DelayedControlModel(0.5, 1.0, 1).name(), "Linear model 1.2");
    EXPECT_STREQ(LogarithmicInteractionModel(0.5, 1.0).name(), "Nonlinear model 2.4");
    EXPECT_STREQ(CubicDifferentialModel(0.5, 1.0, 0.1).name(), "Differential model 3.6");
}

// Boundary values of the delay.
TEST(DelayedControlBoundary, DelayLessThanOneIsRejected) {
    EXPECT_THROW(DelayedControlModel(0.5, 1.0, 0), std::invalid_argument);
    EXPECT_THROW(DelayedControlModel(0.5, 1.0, -3), std::invalid_argument);
    EXPECT_NO_THROW(DelayedControlModel(0.5, 1.0, 1));
}

TEST(DelayedControlBoundary, FirstDelayedInputAppearsExactlyAtTauEqualsDelay) {
    DelayedControlModel model(0.0, 1.0, 3);
    EXPECT_DOUBLE_EQ(model.step(7.0, 0), 0.0);  // tau - k < 0 -> 0
    EXPECT_DOUBLE_EQ(model.step(8.0, 1), 0.0);
    EXPECT_DOUBLE_EQ(model.step(9.0, 2), 0.0);  // tau = k - 1: still nothing
    EXPECT_DOUBLE_EQ(model.step(1.0, 3), 7.0);  // tau = k: u[0] = 7 arrives
    EXPECT_DOUBLE_EQ(model.step(1.0, 4), 8.0);
}

TEST(DelayedControlBoundary, NonSequentialTauThrowsOutOfRange) {
    DelayedControlModel model(0.5, 1.0, 1);
    EXPECT_THROW(model.step(1.0, 5), std::out_of_range);
}

// A substituted (custom) input signal: ramp u(t) = t drives the model.
TEST(DelayedControlCustomSignal, RampInputIsDelayedByK) {
    DelayedControlModel model(0.0, 1.0, 2);
    const double expected[] = {0.0, 0.0, 0.0, 1.0, 2.0, 3.0};  // y(tau+1) = u(tau-2)
    for (int tau = 0; tau < 6; ++tau) {
        EXPECT_DOUBLE_EQ(model.step(static_cast<double>(tau), tau), expected[tau]);
    }
}

// Boundary / special points of the nonlinearity of model 2.4.
TEST(LogarithmicBoundary, ZeroInputGivesZeroForAnyCoefficients) {
    LogarithmicInteractionModel model(3.0, -7.0);
    EXPECT_EQ(model.step(0.0, 0), 0.0);
}

TEST(LogarithmicBoundary, OutputIsEvenInInput) {
    LogarithmicInteractionModel plus(0.7, 1.3), minus(0.7, 1.3);
    EXPECT_DOUBLE_EQ(plus.step(2.5, 0), minus.step(-2.5, 0));
}

TEST(LogarithmicBoundary, CosineVanishesAtHalfPi) {
    // choose u so that y1 = b*ln(1+u) = pi/2; then a*y*cos(y) = 0 and y2 = b*ln(1+0) = 0
    const double kPi = std::acos(-1.0);
    const double b = 1.0;
    LogarithmicInteractionModel model(0.7, b);
    const double y1 = model.step(std::exp(kPi / 2.0 / b) - 1.0, 0);
    EXPECT_NEAR(y1, kPi / 2.0, 1e-12);
    EXPECT_NEAR(model.step(0.0, 1), 0.0, 1e-12);
}

TEST(LogarithmicBoundary, LargeInputGrowsOnlyLogarithmically) {
    LogarithmicInteractionModel model(0.0, 1.0);
    EXPECT_NEAR(model.step(1e12, 0), std::log(1e12), 1e-9);
}

TEST(EulerBoundary, NonPositiveTimeStepIsRejected) {
    EXPECT_THROW(CubicDifferentialModel(1.0, 1.0, 0.0), std::invalid_argument);
    EXPECT_THROW(CubicDifferentialModel(1.0, 1.0, -0.1), std::invalid_argument);
    EXPECT_NO_THROW(CubicDifferentialModel(1.0, 1.0, 1e-6));
}
