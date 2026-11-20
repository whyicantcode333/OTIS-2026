#include <gtest/gtest.h>
#include "Model2_9.h"

// ---------- Тест 1: корень из входа ----------
TEST(Model29Test, SquareRootModulation) {
    Model2_9 model(0.0, 1.0);   // a=0, чтобы y = b*sqrt(|u|)*sign(u)
    model.reset();

    EXPECT_NEAR(model.next(1.0), 1.0, 1e-6);   // sqrt(1) = 1
    EXPECT_NEAR(model.next(4.0), 2.0, 1e-6);   // sqrt(4) = 2
    EXPECT_NEAR(model.next(9.0), 3.0, 1e-6);   // sqrt(9) = 3
    EXPECT_NEAR(model.next(16.0), 4.0, 1e-6);   // sqrt(16) = 4
}

// ---------- Тест 2: знак сохраняется ----------
TEST(Model29Test, SignIsPreserved) {
    Model2_9 model(0.0, 1.0);
    model.reset();
    EXPECT_NEAR(model.next(-4.0), -2.0, 1e-6);  // -sqrt(4) = -2
    EXPECT_NEAR(model.next(4.0), 2.0, 1e-6);
    EXPECT_NEAR(model.next(-9.0), -3.0, 1e-6);
}

// ---------- Тест 3: нулевой вход ----------
TEST(Model29Test, ZeroInputGivesZeroOutput) {
    Model2_9 model(0.0, 1.0);
    model.reset();
    EXPECT_NEAR(model.next(0.0), 0.0, 1e-9);   // sqrt(0) = 0
}

// ---------- Тест 4: name() содержит "2.9" ----------
TEST(Model29Test, NameContainsModelIdentifier) {
    Model2_9 model(0.9, 1.0);
    EXPECT_NE(model.name().find("2.9"), std::string::npos);
}