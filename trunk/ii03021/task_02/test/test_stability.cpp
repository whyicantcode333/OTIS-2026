// Stability analyzer: roots of the characteristic equation on the Z-plane.
#include <gtest/gtest.h>

#include "stability.hpp"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using otis::Stability;

// ------------------------------------------------------------ classification, boundary |z| = 1

TEST(StabilityClassify, InsideAndOutsideUnitCircle) {
    EXPECT_EQ(otis::classifySpectralRadius(0.0), Stability::Stable);
    EXPECT_EQ(otis::classifySpectralRadius(0.5), Stability::Stable);
    EXPECT_EQ(otis::classifySpectralRadius(0.999), Stability::Stable);
    EXPECT_EQ(otis::classifySpectralRadius(1.001), Stability::Unstable);
    EXPECT_EQ(otis::classifySpectralRadius(10.0), Stability::Unstable);
}

TEST(StabilityClassify, UnitCircleIsMarginal) {
    EXPECT_EQ(otis::classifySpectralRadius(1.0), Stability::Marginal);
    EXPECT_EQ(otis::classifySpectralRadius(1.0 + 1e-12), Stability::Marginal);
    EXPECT_EQ(otis::classifySpectralRadius(1.0 - 1e-12), Stability::Marginal);
}

TEST(StabilityClassify, CustomToleranceMovesTheBoundary) {
    EXPECT_EQ(otis::classifySpectralRadius(1.01, 0.0), Stability::Unstable);
    EXPECT_EQ(otis::classifySpectralRadius(1.01, 0.05), Stability::Marginal);
}

TEST(StabilityClassify, NotANumberIsUnstable) {
    EXPECT_EQ(otis::classifySpectralRadius(std::numeric_limits<double>::quiet_NaN()),
              Stability::Unstable);
}

TEST(StabilityRadius, MaxModulusOfComplexRoots) {
    using C = std::complex<double>;
    EXPECT_DOUBLE_EQ(otis::spectralRadius({}), 0.0);
    EXPECT_DOUBLE_EQ(otis::spectralRadius({C(0.5, 0.0), C(-2.0, 0.0)}), 2.0);
    EXPECT_DOUBLE_EQ(otis::spectralRadius({C(3.0, 4.0), C(1.0, 0.0)}), 5.0);
}

TEST(StabilityNames, AllStatusesHaveNames) {
    EXPECT_STREQ(otis::stabilityName(Stability::Stable), "stable");
    EXPECT_STREQ(otis::stabilityName(Stability::Marginal), "marginal");
    EXPECT_STREQ(otis::stabilityName(Stability::Unstable), "unstable");
    EXPECT_STREQ(otis::stabilityName(static_cast<Stability>(42)), "unknown");
}

// ------------------------------------------------------------ model 1.2 (TEST_P)

class StableDelayedTest : public ::testing::TestWithParam<double> {};

TEST_P(StableDelayedTest, AbsAUnderOneIsStable) {
    const double a = GetParam();
    const auto report = otis::analyzeDelayedControl(a, 3);
    EXPECT_EQ(report.status, Stability::Stable);
    EXPECT_TRUE(report.isStable());
    EXPECT_NEAR(report.spectralRadius, std::abs(a), 1e-12);
}

INSTANTIATE_TEST_SUITE_P(InsideUnitCircle, StableDelayedTest,
                         ::testing::Values(-0.99, -0.5, 0.0, 0.5, 0.99));

class UnstableDelayedTest : public ::testing::TestWithParam<double> {};

TEST_P(UnstableDelayedTest, AbsAOverOneIsUnstable) {
    const double a = GetParam();
    const auto report = otis::analyzeDelayedControl(a, 1);
    EXPECT_EQ(report.status, Stability::Unstable);
    EXPECT_FALSE(report.isStable());
    EXPECT_NEAR(report.spectralRadius, std::abs(a), 1e-9);
}

INSTANTIATE_TEST_SUITE_P(OutsideUnitCircle, UnstableDelayedTest,
                         ::testing::Values(-1000.0, -2.0, -1.01, 1.01, 2.0, 1000.0));

class MarginalDelayedTest : public ::testing::TestWithParam<double> {};

TEST_P(MarginalDelayedTest, AbsAEqualOneIsMarginalAndNotStable) {
    const auto report = otis::analyzeDelayedControl(GetParam(), 2);
    EXPECT_EQ(report.status, Stability::Marginal);
    EXPECT_FALSE(report.isStable());
}

INSTANTIATE_TEST_SUITE_P(OnUnitCircle, MarginalDelayedTest, ::testing::Values(-1.0, 1.0));

// The delay adds only roots at z = 0, so it never changes stability.
class DelayIndependenceTest : public ::testing::TestWithParam<int> {};

TEST_P(DelayIndependenceTest, DelayDoesNotChangeVerdict) {
    const int k = GetParam();
    EXPECT_EQ(otis::analyzeDelayedControl(0.7, k).status, Stability::Stable);
    EXPECT_EQ(otis::analyzeDelayedControl(1.0, k).status, Stability::Marginal);
    EXPECT_EQ(otis::analyzeDelayedControl(1.3, k).status, Stability::Unstable);
}

INSTANTIATE_TEST_SUITE_P(Delays, DelayIndependenceTest, ::testing::Values(1, 2, 5, 1000000));

TEST(StabilityDelayed, RootsAreAAndZero) {
    const auto report = otis::analyzeDelayedControl(-0.4, 2);
    ASSERT_EQ(report.roots.size(), 2u);
    EXPECT_DOUBLE_EQ(report.roots[0].real(), -0.4);
    EXPECT_DOUBLE_EQ(report.roots[0].imag(), 0.0);
    EXPECT_DOUBLE_EQ(std::abs(report.roots[1]), 0.0);
}

TEST(StabilityDelayed, EquationAndMessageAreFilled) {
    const auto k1 = otis::analyzeDelayedControl(0.5, 1);
    const auto k3 = otis::analyzeDelayedControl(0.5, 3);
    EXPECT_NE(k1.equation.find("z * (z - 0.5)"), std::string::npos) << k1.equation;
    EXPECT_NE(k3.equation.find("z^3 * (z - 0.5)"), std::string::npos) << k3.equation;
    EXPECT_FALSE(k1.message.empty());
    EXPECT_FALSE(otis::analyzeDelayedControl(1.0, 1).message.empty());
    EXPECT_FALSE(otis::analyzeDelayedControl(2.0, 1).message.empty());
}

TEST(StabilityDelayed, InvalidArgumentsThrow) {
    EXPECT_THROW(otis::analyzeDelayedControl(0.5, 0), std::invalid_argument);
    EXPECT_THROW(otis::analyzeDelayedControl(0.5, -1), std::invalid_argument);
    EXPECT_THROW(otis::analyzeDelayedControl(std::numeric_limits<double>::quiet_NaN(), 1),
                 std::invalid_argument);
    EXPECT_THROW(otis::analyzeDelayedControl(std::numeric_limits<double>::infinity(), 1),
                 std::invalid_argument);
}

// ------------------------------------------------------------ model 2.4 (linearization)

TEST(StabilityLogarithmic, VerdictFollowsAbsA) {
    EXPECT_EQ(otis::analyzeLogarithmic(0.5).status, Stability::Stable);
    EXPECT_EQ(otis::analyzeLogarithmic(-0.99).status, Stability::Stable);
    EXPECT_EQ(otis::analyzeLogarithmic(1.0).status, Stability::Marginal);
    EXPECT_EQ(otis::analyzeLogarithmic(-1.0).status, Stability::Marginal);
    EXPECT_EQ(otis::analyzeLogarithmic(1.5).status, Stability::Unstable);
    EXPECT_NE(otis::analyzeLogarithmic(0.5).equation.find("linearization"), std::string::npos);
    EXPECT_THROW(otis::analyzeLogarithmic(std::numeric_limits<double>::infinity()),
                 std::invalid_argument);
}

// ------------------------------------------------------------ console warning

TEST(StabilityPrint, StableProcessPrintsNoWarning) {
    std::ostringstream out;
    const bool warned = otis::printStabilityReport(otis::analyzeDelayedControl(0.5, 2),
                                                   "Linear model 1.2", out);
    EXPECT_FALSE(warned);
    EXPECT_EQ(out.str().find("WARNING"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("[OK]"), std::string::npos);
    EXPECT_NE(out.str().find("Linear model 1.2"), std::string::npos);
}

TEST(StabilityPrint, DivergentProcessPrintsWarning) {
    std::ostringstream out;
    const bool warned = otis::printStabilityReport(otis::analyzeDelayedControl(1.5, 1),
                                                   "Linear model 1.2", out);
    EXPECT_TRUE(warned);
    EXPECT_NE(out.str().find("[WARNING]"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("unstable"), std::string::npos);
    EXPECT_NE(out.str().find("z1 = 1.5"), std::string::npos);
}

TEST(StabilityPrint, BoundaryCasePrintsWarning) {
    std::ostringstream out;
    EXPECT_TRUE(otis::printStabilityReport(otis::analyzeDelayedControl(-1.0, 1), "m", out));
    EXPECT_NE(out.str().find("marginally"), std::string::npos) << out.str();
}

TEST(StabilityPrint, ComplexRootsAreFormatted) {
    otis::StabilityReport report;
    report.roots = {{0.3, 0.4}, {0.3, -0.4}};
    report.spectralRadius = otis::spectralRadius(report.roots);
    report.status = otis::classifySpectralRadius(report.spectralRadius);
    report.equation = "z^2 - 0.6z + 0.25 = 0";
    report.message = "test";
    std::ostringstream out;
    EXPECT_FALSE(otis::printStabilityReport(report, "complex", out));
    EXPECT_NE(out.str().find("0.3 + 0.4i"), std::string::npos) << out.str();
    EXPECT_NE(out.str().find("0.3 - 0.4i"), std::string::npos) << out.str();
}
