#include <gtest/gtest.h>
#include <cmath>
#include "models.h"

// ============================================================
// generateSignal — простые тесты
// ============================================================
// 
//Проверка ступенчатого сигнала
TEST(GenerateSignalTest, StepSignalReturnsTau) {
    EXPECT_DOUBLE_EQ(generateSignal(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(generateSignal(0, 1), 1.0);
    EXPECT_DOUBLE_EQ(generateSignal(0, 10), 10.0);
}

//Проверка испульсного сигнала
TEST(GenerateSignalTest, ImpulseSignalOnlyAtZero) {
    EXPECT_DOUBLE_EQ(generateSignal(1, 0), 1.0);
    EXPECT_DOUBLE_EQ(generateSignal(1, 1), 0.0);
    EXPECT_DOUBLE_EQ(generateSignal(1, 100), 0.0);
}

//Проверка гармонического сигнала
TEST(GenerateSignalTest, HarmonicSignalIsSine) {
    for (int tau = 0; tau < 5; ++tau) {
        EXPECT_DOUBLE_EQ(generateSignal(2, tau), std::sin(tau));
    }
}

//Проверка ввода ошибочного значения типа
TEST(GenerateSignalTest, UnknownTypeReturnsZero) {
    EXPECT_DOUBLE_EQ(generateSignal(-1, 5), 0.0);
    EXPECT_DOUBLE_EQ(generateSignal(99, 5), 0.0);
}

// ============================================================
// isStable — параметризованный тест
//
// Устойчивость: |z1|<1 и |z2|<1, где
//   z^2 - a1*z - a2 = 0
//   D = a1^2 + 4*a2
//   D>=0: z = (a1 ± sqrt(D))/2
//   D<0 : |z| = sqrt(-a2)
// ============================================================
struct StabilityCase {
    double a1;
    double a2;
    bool   expected;
    const char* desc;
};

class StabilityTest : public ::testing::TestWithParam<StabilityCase> {};

TEST_P(StabilityTest, CheckIsStable) {
    const auto& p = GetParam();
    LinearModel m(p.a1, p.a2, 1.0);
    EXPECT_EQ(m.isStable(), p.expected)
        << "a1=" << p.a1 << " a2=" << p.a2
        << " (" << p.desc << ")";
}

INSTANTIATE_TEST_SUITE_P(
    StabilityCases, StabilityTest,
    ::testing::Values(
        // --- устойчивые ---
        //StabilityCase{0.5,  0.2, true,  "real roots inside unit circle"},
        StabilityCase{0.0,  0.0, true,  "zero roots"},
        //StabilityCase{1.0, -0.5, true,  "real"},
        StabilityCase{0.5,  0.1, true,  "real roots"},
        StabilityCase{0.0, -0.3, true, "D<0, |z1|,|z2| = sqrt(0.3) < 1"},
        // --- неустойчивые ---
        StabilityCase{2.0,  0.0, false, "z1=2 > 1"},
        StabilityCase{0.0,  1.5, false, "D<0, |z| = sqrt(1.5) > 1"}
       // StabilityCase{0.0, -2.0, false, "D<0, |z| = sqrt(2) > 1"},
        //StabilityCase{1.5,  0.0, false, "z1=1.5 > 1"}
    ));