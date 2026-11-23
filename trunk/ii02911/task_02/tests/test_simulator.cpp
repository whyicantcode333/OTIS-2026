
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

#include "Simulator.h"
#include "Model1_9.h"
#include "Model3_9.h"

using ::testing::_;
using ::testing::Return;

class MockSignalSource : public ISignalSource
{
public:
    MOCK_METHOD(double, sample, (int step, double amplitude), (const, override));
};

TEST(SimulatorMockTest, UsesMockedSignalSequenceDuringSimulation)
{
    MockSignalSource mockSignal;
    Model1_9 model(0.5, 0.0, 0.0, 1.0);

    EXPECT_CALL(mockSignal, sample(1, 2.0)).WillOnce(Return(2.0));
    EXPECT_CALL(mockSignal, sample(2, 2.0)).WillOnce(Return(0.0));
    EXPECT_CALL(mockSignal, sample(3, 2.0)).WillOnce(Return(1.0));

    const auto history = Simulator::run(model, mockSignal, 3, 2.0, 0.1, 0.0);

    ASSERT_EQ(history.size(), 4u);
    EXPECT_DOUBLE_EQ(history[0], 0.0);
    EXPECT_DOUBLE_EQ(history[1], 2.0);
    EXPECT_DOUBLE_EQ(history[2], 1.0);
    EXPECT_DOUBLE_EQ(history[3], 1.5);
}

TEST(SimulatorTest, ThrowsOnNegativeSteps)
{
    MockSignalSource mockSignal;
    Model3_9 model(1.0);

    EXPECT_THROW(Simulator::run(model, mockSignal, -1, 1.0, 0.1), std::invalid_argument);
}

TEST(SimulatorTest, RealFunctionSignalSourceRunsCorrectly)
{
    FunctionSignalSource stepSource(SignalType::Step);
    Model3_9 model(1.0);

    const auto history = Simulator::run(model, stepSource, 2, 0.0, 0.5, 3.0);
    ASSERT_EQ(history.size(), 3u);
    EXPECT_DOUBLE_EQ(history[2], 3.0);
}
