#include <gtest/gtest.h>
#include "Model1_4.h"
#include "Model2_9.h"
#include "Model3_1.h"

// ============================================================
// Model 1.4 — устойчивость: |a| < 1
// ============================================================
TEST(Model14Stability, StableCoefficient) {
    Model1_4 model(0.9, 1.0);
    EXPECT_TRUE(model.isStable());
    EXPECT_TRUE(model.stabilityWarning().empty());
}

TEST(Model14Stability, UnstableCoefficient) {
    Model1_4 model(1.2, 1.0);
    EXPECT_FALSE(model.isStable());
    EXPECT_FALSE(model.stabilityWarning().empty());
}

TEST(Model14Stability, BoundaryIsNotStable) {
    Model1_4 model(1.0, 1.0);
    EXPECT_FALSE(model.isStable());
    const std::string warn = model.stabilityWarning();
    EXPECT_FALSE(warn.empty());
    EXPECT_NE(warn.find("boundary"), std::string::npos);
}

TEST(Model14Stability, NegativeStable) {
    Model1_4 model(-0.9, 1.0);
    EXPECT_TRUE(model.isStable());    // |−0.9| = 0.9 < 1
}

// ============================================================
// Model 3.1 — устойчивость схемы Эйлера: |1 − a·Δt| < 1
// ============================================================
TEST(Model31Stability, SmallDtIsStable) {
    Model3_1 model(1.0, 0.1);
    EXPECT_TRUE(model.isStable());
    EXPECT_TRUE(model.stabilityWarning().empty());
}

TEST(Model31Stability, LargeDtIsUnstable) {
    Model3_1 model(1.0, 3.0);         // |1 − 3| = 2 > 1
    EXPECT_FALSE(model.isStable());
    EXPECT_FALSE(model.stabilityWarning().empty());
}

TEST(Model31Stability, CriticalDtAtBoundary) {
    Model3_1 model(1.0, 2.0);         // |1 − 2| = 1 — граница
    EXPECT_FALSE(model.isStable());   // НЕ строго меньше
}

// ============================================================
// Model 2.9 — нелинейная, isStable() всегда true
// ============================================================
TEST(Model29Stability, AlwaysReportsStable) {
    Model2_9 model(100.0, 1.0);       // даже при "плохих" коэффициентах
    EXPECT_TRUE(model.isStable());
    EXPECT_TRUE(model.stabilityWarning().empty());
}

// ============================================================
// Тест базовой реализации Model (для покрытия Model.h)
// ============================================================
class DummyModel : public Model {
public:
    double next(double /*u*/) override { return 0.0; }
    void reset() override {}
    std::string name() const override { return "Dummy"; }
    // isStable() и stabilityWarning() НЕ переопределяем
};

TEST(BaseModelTest, DefaultStabilityIsTrue) {
    DummyModel model;
    EXPECT_TRUE(model.isStable());
}

TEST(BaseModelTest, DefaultWarningIsEmpty) {
    DummyModel model;
    EXPECT_TRUE(model.stabilityWarning().empty());
}