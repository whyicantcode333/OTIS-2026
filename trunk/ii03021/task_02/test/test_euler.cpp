// Block 3: explicit Euler step of dy/dt = a*y^3 + b*u compared with analytic solutions.
#include <gtest/gtest.h>

#include "euler_ivp.hpp"
#include "models.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

using otis::CubicDifferentialModel;
using otis::CubicEulerIvpModel;

namespace {

// dy/dt = -y^3, y(0) = 1  ->  y(t) = 1 / sqrt(1 + 2t); y(1) = 1/sqrt(3)
double eulerAtOne(double dt) {
    CubicEulerIvpModel model(-1.0, 0.0, dt, 1.0);
    const int n = static_cast<int>(std::lround(1.0 / dt));
    double y = 0.0;
    for (int tau = 0; tau < n; ++tau) y = model.step(0.0, tau);
    return y;
}

const double kAnalytic = 1.0 / std::sqrt(3.0);

} // namespace

class EulerVsAnalyticTest : public ::testing::TestWithParam<double> {};

// Euler is a first-order method: global error <= C*dt (C = 0.2 holds for this problem).
TEST_P(EulerVsAnalyticTest, ErrorIsFirstOrderInStep) {
    const double dt = GetParam();
    EXPECT_NEAR(eulerAtOne(dt), kAnalytic, 0.2 * dt);
}

INSTANTIATE_TEST_SUITE_P(TimeSteps, EulerVsAnalyticTest,
                         ::testing::Values(0.1, 0.05, 0.025, 0.0125, 0.00625));

TEST(EulerVsAnalytic, HalvingStepHalvesTheError) {
    const double e1 = std::abs(eulerAtOne(0.05) - kAnalytic);
    const double e2 = std::abs(eulerAtOne(0.025) - kAnalytic);
    EXPECT_GT(e1 / e2, 1.8);
    EXPECT_LT(e1 / e2, 2.2);
}

// With a = 0 the equation is dy/dt = b*u: Euler is exact, y(n*dt) = b*u*n*dt.
struct LinearCase {
    double b, dt;
    int n;
};

class EulerExactForLinearTest : public ::testing::TestWithParam<LinearCase> {};

TEST_P(EulerExactForLinearTest, StepInputGivesStraightLine) {
    const LinearCase c = GetParam();
    CubicDifferentialModel model(0.0, c.b, c.dt);
    double y = 0.0;
    for (int tau = 0; tau < c.n; ++tau) y = model.step(1.0, tau);
    EXPECT_NEAR(y, c.b * c.n * c.dt, 1e-9);
}

INSTANTIATE_TEST_SUITE_P(Lines, EulerExactForLinearTest,
                         ::testing::Values(LinearCase{1.0, 0.1, 10}, LinearCase{3.0, 0.5, 4},
                                           LinearCase{-2.0, 0.01, 100}, LinearCase{0.5, 1.0, 7}));

// Equilibrium: a*y^3 + b*u = 0 -> y* = cbrt(-b*u/a). Starting there, Euler must not move.
TEST(EulerInvariants, EquilibriumIsAFixedPoint) {
    const double a = -2.0, b = 2.0;       // y* = cbrt(1) = 1
    CubicEulerIvpModel model(a, b, 0.1, 1.0);
    for (int tau = 0; tau < 20; ++tau) {
        EXPECT_NEAR(model.step(1.0, tau), 1.0, 1e-12);
    }
}

TEST(EulerInvariants, InitialStateIsRestoredByReset) {
    CubicEulerIvpModel model(-1.0, 0.0, 0.1, 1.0);
    const double first = model.step(0.0, 0);
    model.step(0.0, 1);
    model.reset();
    EXPECT_DOUBLE_EQ(model.step(0.0, 0), first);
}

TEST(EulerInvariants, RestStateWithZeroInputStaysAtZero) {
    CubicDifferentialModel model(5.0, 5.0, 0.3);
    for (int tau = 0; tau < 20; ++tau) EXPECT_EQ(model.step(0.0, tau), 0.0);
}

TEST(EulerInvariants, ZeroInitialStateMatchesLab1Model) {
    CubicEulerIvpModel ivp(-1.0, 1.0, 0.1, 0.0);
    CubicDifferentialModel lab1(-1.0, 1.0, 0.1);
    for (int tau = 0; tau < 30; ++tau) {
        EXPECT_DOUBLE_EQ(ivp.step(1.0, tau), lab1.step(1.0, tau));
    }
}

TEST(EulerInvariants, IvpModelRejectsNonPositiveStepAndHasName) {
    EXPECT_THROW(CubicEulerIvpModel(1.0, 1.0, 0.0, 1.0), std::invalid_argument);
    EXPECT_THROW(CubicEulerIvpModel(1.0, 1.0, -1.0, 1.0), std::invalid_argument);
    EXPECT_NE(std::string(CubicEulerIvpModel(1.0, 1.0, 0.1, 1.0).name()).find("3.6"), std::string::npos);
}
