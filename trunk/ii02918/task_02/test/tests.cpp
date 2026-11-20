#include <gtest/gtest.h>
#include "StateDelayModel.h"
#include "SaturationModel.h"
#include "QuadraticDecayODE.h"
#include "ModelCreator.h"
#include "StabilityAnalyzer.h"
#include "SignalGenerator.h"
#include "MockSignal.h"

TEST(Model1_5_Test, CorrectStepCalculation) {
    StateDelayModel model(0.4, 0.3, 1.0, 1);
    
    EXPECT_DOUBLE_EQ(model.advance(1.0), 1.0);
    
    EXPECT_DOUBLE_EQ(model.advance(1.0), 1.4);
    
    EXPECT_DOUBLE_EQ(model.advance(1.0), 1.86);
}

TEST(StabilityAnalyzer_Test, StableCoefficients) {
    EXPECT_TRUE(StabilityAnalyzer::isStableStateDelay(0.4, 0.3));
}

TEST(StabilityAnalyzer_Test, UnstableCoefficients) {
    EXPECT_FALSE(StabilityAnalyzer::isStableStateDelay(0.6, 0.5));
}

TEST(StabilityAnalyzer_Test, EulerMethodStability) {
    EXPECT_TRUE(StabilityAnalyzer::isStableEuler(0.5, 0.1, 0.1, 1.0));

    EXPECT_FALSE(StabilityAnalyzer::isStableEuler(0.5, 0.1, 3.0, 1.0));
}

TEST(Saturation_Test, BoundaryValues) {
    SaturationModel model(0.5, 1.0, -1.0, 1.0);
    model.reset(); 
    
    EXPECT_DOUBLE_EQ(model.advance(1.0), 1.0);
    
    model.reset(); 
    EXPECT_DOUBLE_EQ(model.advance(-1.0), -1.0);
    
    model.reset(); 
    EXPECT_DOUBLE_EQ(model.advance(5.0), 1.0);
}

TEST(Invariants_Test, ZeroInputZeroState) {
    StateDelayModel model(0.4, 0.3, 1.0, 2);
    model.reset();
    EXPECT_DOUBLE_EQ(model.advance(0.0), 0.0);
    EXPECT_DOUBLE_EQ(model.advance(0.0), 0.0);
}

TEST(Signals_Test, StepSignalCorrectness) {
    StepSignal signal(5.0); 
    EXPECT_DOUBLE_EQ(signal.value(1), 5.0);
    EXPECT_DOUBLE_EQ(signal.value(10), 5.0);
}

class Model2_2_ParamTest : public ::testing::TestWithParam<std::tuple<double, double>> {
};

TEST_P(Model2_2_ParamTest, SaturationCheck) {
    auto [input_u, expected_y] = GetParam();
    
    SaturationModel model(0.5, 1.0, -1.0, 1.0);
    model.reset();
    
    EXPECT_DOUBLE_EQ(model.advance(input_u), expected_y);
}

INSTANTIATE_TEST_SUITE_P(
    SaturationBoundaries,
    Model2_2_ParamTest,
    ::testing::Values(
        std::make_tuple(5.0, 1.0), 
        std::make_tuple(-5.0, -1.0), 
        std::make_tuple(0.5, 0.5)    
    )
);

TEST(MockSignalTest, SimulateWithMock) {
    MockSignal mockSignal;
    
    EXPECT_CALL(mockSignal, getValue(1)).WillOnce(::testing::Return(1.0));
    EXPECT_CALL(mockSignal, getValue(2)).WillOnce(::testing::Return(0.0));

    StateDelayModel model(0.4, 0.3, 1.0, 1);
    model.reset();
    
    double u1 = mockSignal.getValue(1);
    EXPECT_DOUBLE_EQ(model.advance(u1), 1.0);
    
    double u2 = mockSignal.getValue(2);
    EXPECT_DOUBLE_EQ(model.advance(u2), 0.4); 
}

TEST(Model3_8_Test, EulerCalculation) {
    QuadraticDecayODE model(0.5, 0.1, 1.0, 0.1);
    model.reset(); // y = 1.0

    EXPECT_NEAR(model.advance(1.0), 1.04, 1e-5);

    EXPECT_NEAR(model.advance(1.0), 1.07718, 1e-5);
}

TEST(ModelCreator_Test, CreatesCorrectModels) {

    StateDelayModel m1(0.4, 0.3, 1.0, 1);
    EXPECT_EQ(m1.describe(), "Model 1.5 (State Delay, k=1)");

    SaturationModel m2(0.5, 1.0, -1.0, 1.0);
    EXPECT_EQ(m2.describe(), "Model 2.2 (Saturation, Umin=-1, Umax=1)");
}

TEST(Signals_Test, BurstSignalCorrectness) {
    BurstSignal signal;
    EXPECT_DOUBLE_EQ(signal.value(1), 1.0);
    EXPECT_DOUBLE_EQ(signal.value(2), 0.0);
    EXPECT_DOUBLE_EQ(signal.value(10), 0.0);
}

TEST(Signals_Test, HarmonicSignalCorrectness) {
    HarmonicSignal signal;
    EXPECT_NEAR(signal.value(0), 0.0, 1e-5);
    EXPECT_NEAR(signal.value(1), 0.84147, 1e-4);
}

TEST(Model3_8_Test, ResetAndStability) {
    QuadraticDecayODE model(0.5, 0.1, 1.0, 0.1);
    model.advance(1.0); 
    model.reset();      

    EXPECT_NEAR(model.advance(0.0), 0.94, 1e-5);

    EXPECT_TRUE(model.verifyStability());

    QuadraticDecayODE unstableModel(0.5, 0.1, 1.0, 10.0);
    EXPECT_FALSE(unstableModel.verifyStability());
}

TEST(ModelCreator_Test, FactoryCreatesObjects) {
    StateDelayModel m1(0.1, 0.2, 0.3, 2);
    EXPECT_EQ(m1.describe(), "Model 1.5 (State Delay, k=2)");

    SaturationModel m2(0.1, 0.2, -5.0, 5.0);
    EXPECT_EQ(m2.describe(), "Model 2.2 (Saturation, Umin=-5, Umax=5)");

    QuadraticDecayODE m3(0.1, 0.2, 0.3, 0.01);
    EXPECT_EQ(m3.describe(), "Model 3.8 (Quadratic Decay ODE, dt=0.01)");
}

TEST(ModelCreator_Test, CreateStateDelayModelViaFactory) {
    std::string input = "0.4\n0.3\n1.0\n2\n"; 
    std::istringstream fake_cin(input);
    std::streambuf* old_cin_buf = std::cin.rdbuf(fake_cin.rdbuf()); 

    auto model = ModelCreator::create(1); 
    
    std::cin.rdbuf(old_cin_buf); 

    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->describe(), "Model 1.5 (State Delay, k=2)");
}

TEST(ModelCreator_Test, CreateSaturationModelViaFactory) {
    std::string input = "0.5\n1.0\n-1.0\n1.0\n";
    std::istringstream fake_cin(input);
    std::streambuf* old_cin_buf = std::cin.rdbuf(fake_cin.rdbuf());

    auto model = ModelCreator::create(2); 
    
    std::cin.rdbuf(old_cin_buf);

    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->describe(), "Model 2.2 (Saturation, Umin=-1, Umax=1)");
}

TEST(ModelCreator_Test, CreateQuadraticDecayModelViaFactory) {
    std::string input = "0.5\n0.1\n1.0\n0.1\n"; 
    std::istringstream fake_cin(input);
    std::streambuf* old_cin_buf = std::cin.rdbuf(fake_cin.rdbuf());

    auto model = ModelCreator::create(3); 
    
    std::cin.rdbuf(old_cin_buf);

    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->describe(), "Model 3.8 (Quadratic Decay ODE, dt=0.1)");
}

TEST(Signals_Test, NamesAreCorrect) {
    StepSignal s1(1.0);
    EXPECT_EQ(s1.getName(), "Step (u = const)");

    BurstSignal s2;
    EXPECT_EQ(s2.getName(), "Burst (impulse at tau=1)");

    HarmonicSignal s3;
    EXPECT_EQ(s3.getName(), "Harmonic (u = sin(tau))");
}

class DummyModel : public DynamicModel {
public:
    double advance(double) override { return 0.0; }
    void reset() override {}
    std::string describe() const override { return "Dummy"; }
};

TEST(DynamicModel_Test, Defaults) {
    DummyModel dummy;
    EXPECT_TRUE(dummy.verifyStability());
    EXPECT_EQ(dummy.getWarningMessage(), "");
}