#include <gtest/gtest.h>
#include "Model1_4.h"

// ---------- Тест 1: значения при Step-входе ----------
TEST(Model14Test, StepResponseMatchesAnalyticalSolution) {
    Model1_4 model(0.9, 1.0);
    model.reset();

    // Step: u = 1 на каждом шаге
    // Ожидание: y_tau = 0.9^(tau-1) для tau >= 1
    EXPECT_NEAR(model.next(1.0), 1.0, 1e-6);   // y1
    EXPECT_NEAR(model.next(1.0), 0.9, 1e-6);   // y2
    EXPECT_NEAR(model.next(1.0), 0.81, 1e-6);   // y3
    EXPECT_NEAR(model.next(1.0), 0.729, 1e-6);   // y4
    EXPECT_NEAR(model.next(1.0), 0.6561, 1e-6);   // y5
}

// ---------- Тест 2: при нулевом входе система в покое ----------
TEST(Model14Test, ZeroInputKeepsZeroState) {
    Model1_4 model(0.9, 1.0);
    model.reset();

    // Если y=0 и u=0 — y остаётся 0
    EXPECT_NEAR(model.next(0.0), 0.0, 1e-9);
    EXPECT_NEAR(model.next(0.0), 0.0, 1e-9);
    EXPECT_NEAR(model.next(0.0), 0.0, 1e-9);
}

// ---------- Тест 3: reset() возвращает в начальное ----------
TEST(Model14Test, ResetRestoresInitialState) {
    Model1_4 model(0.9, 1.0);
    model.reset();
    // Сдвигаем состояние
    model.next(1.0);
    model.next(1.0);
    // Сбрасываем
    model.reset();
    // После reset первое значение снова 1.0
    EXPECT_NEAR(model.next(1.0), 1.0, 1e-6);
}

// ---------- Тест 4: name() содержит "1.4" ----------
TEST(Model14Test, NameContainsModelIdentifier) {
    Model1_4 model(0.9, 1.0);
    EXPECT_NE(model.name().find("1.4"), std::string::npos);
}