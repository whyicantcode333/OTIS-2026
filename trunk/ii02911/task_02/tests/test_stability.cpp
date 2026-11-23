#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include <sstream>

#include "StabilityChecker.h"

using ::testing::HasSubstr;

TEST(StabilityCheckerTest, StableRealRootsInsideUnitCircle)
{
    // Корни 0.5, 0.2, -0.1:
    // (z - 0.5)(z - 0.2)(z + 0.1) = z^3 - 0.6z^2 + 0.03z + 0.01
    // a1 = 0.6, a2 = -0.03, a3 = -0.01
    EXPECT_TRUE(StabilityChecker::isModel1_9Stable(0.6, -0.03, -0.01));
}

TEST(StabilityCheckerTest, StableComplexConjugateRoots)
{
    // Корни 0.4 +- 0.4i (|z| = sqrt(0.32) < 1) и 0.1
    // (z^2 - 0.8z + 0.32)(z - 0.1) = z^3 - 0.9z^2 + 0.4z - 0.032
    EXPECT_TRUE(StabilityChecker::isModel1_9Stable(0.9, -0.4, 0.032));
}

TEST(StabilityCheckerTest, UnstableWhenRootOutsideUnitCircle)
{
    // Корни 1.2, 0.1, 0.0 -> (z - 1.2)(z - 0.1)z = z^3 - 1.3z^2 + 0.12z
    EXPECT_FALSE(StabilityChecker::isModel1_9Stable(1.3, -0.12, 0.0));
}

TEST(StabilityCheckerTest, UnstableOnUnitCircleBoundary)
{
    // Корень z = 1 (a1 = 1.0, a2 = 0, a3 = 0)
    EXPECT_FALSE(StabilityChecker::isModel1_9Stable(1.0, 0.0, 0.0));
    // Корень z = -1 (a1 = -1.0, a2 = 0, a3 = 0)
    EXPECT_FALSE(StabilityChecker::isModel1_9Stable(-1.0, 0.0, 0.0));
}

TEST(StabilityCheckerTest, UnstableOnNonFiniteCoefficients)
{
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_FALSE(StabilityChecker::isModel1_9Stable(inf, 0.1, 0.1));
    EXPECT_FALSE(StabilityChecker::isModel1_9Stable(0.2, nan, 0.1));
}

TEST(StabilityCheckerTest, CheckAndWarnPrintsMessageOnlyWhenUnstable)
{
    std::ostringstream okOut;
    EXPECT_TRUE(StabilityChecker::checkAndWarn(0.5, 0.1, 0.05, okOut));
    EXPECT_TRUE(okOut.str().empty());

    std::ostringstream warnOut;
    EXPECT_FALSE(StabilityChecker::checkAndWarn(1.5, 0.2, 0.1, warnOut));
    EXPECT_THAT(warnOut.str(), HasSubstr("неустойчива"));
}