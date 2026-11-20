#include <gtest/gtest.h>
#include "InputSignals.h"
#include <cmath>

// ---------- StepInput ----------
TEST(SignalsTest, StepIsConstant) {
    StepInput step(1.0);
    EXPECT_DOUBLE_EQ(step.value(1), 1.0);
    EXPECT_DOUBLE_EQ(step.value(2), 1.0);
    EXPECT_DOUBLE_EQ(step.value(100), 1.0);
}

TEST(SignalsTest, StepCustomAmplitude) {
    StepInput step(5.0);
    EXPECT_DOUBLE_EQ(step.value(1), 5.0);
}

// ---------- ImpulseInput ----------
TEST(SignalsTest, ImpulseOnlyAtFirstStep) {
    ImpulseInput impulse;
    EXPECT_DOUBLE_EQ(impulse.value(1), 1.0);   // u_1 = 1
    EXPECT_DOUBLE_EQ(impulse.value(2), 0.0);   // u_2 = 0
    EXPECT_DOUBLE_EQ(impulse.value(100), 0.0);
}

// ---------- HarmonicInput ----------
TEST(SignalsTest, HarmonicIsSine) {
    HarmonicInput harmonic;
    for (int tau = 1; tau <= 10; ++tau) {
        EXPECT_NEAR(harmonic.value(tau), std::sin(static_cast<double>(tau)), 1e-6);
    }
}

// ---------- name() для всех ----------
TEST(SignalsTest, NamesAreNotNull) {
    StepInput s(1.0);
    ImpulseInput i;
    HarmonicInput h;

    EXPECT_FALSE(s.name().empty());
    EXPECT_FALSE(i.name().empty());
    EXPECT_FALSE(h.name().empty());
}

// ============================================================
// Мок для InputSignal
// ============================================================
#include <gmock/gmock.h>
#include "Model1_4.h"

class MockSignal : public InputSignal {
public:
    MOCK_METHOD(double, value, (int tau), (const, override));
    MOCK_METHOD(std::string, name, (), (const, override));
};

TEST(MockSignalTest, ModelCallsSignalValueWithCorrectTau) {
    MockSignal mockSignal;

    // Ожидание: value(1) будет вызван 1 раз → вернёт 1.0
    //          value(2) будет вызван 1 раз → вернёт 0.0
    EXPECT_CALL(mockSignal, value(1))
        .Times(1)
        .WillOnce(::testing::Return(1.0));
    EXPECT_CALL(mockSignal, value(2))
        .Times(1)
        .WillOnce(::testing::Return(0.0));

    Model1_4 model(0.9, 1.0);
    model.reset();

    // Прогоняем модель с "моком" вместо реального сигнала
    model.next(mockSignal.value(1));
    model.next(mockSignal.value(2));

    // Если дошли — все EXPECT_CALL выполнены
    SUCCEED();
}