// Simulator tests. A Google Mock model replaces the real model, so the control modes
// (step / impulse / harmonic input) can be verified through the calls the simulator makes.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "models.hpp"
#include "signals.hpp"
#include "simulator.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using ::testing::_;
using ::testing::DoubleEq;
using ::testing::NiceMock;
using ::testing::Return;
using otis::SignalType;

namespace {

class MockModel : public otis::Model {
public:
    MOCK_METHOD(double, step, (double input, int tau), (override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(const char*, name, (), (const, override));
};

// Redirects std::cout into a string for the lifetime of the object.
class CoutCapture {
public:
    CoutCapture() : old_(std::cout.rdbuf(buffer_.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(old_); }
    [[nodiscard]] std::string str() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* old_;
};

class TempDirTest : public ::testing::Test {
protected:
    void SetUp() override {
        static int counter = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        dir_ = fs::temp_directory_path() /
               ("otis_tests_" + std::to_string(stamp) + "_" + std::to_string(counter++));
        fs::create_directories(dir_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(dir_, ec);
    }

    [[nodiscard]] std::vector<std::string> readLines(const fs::path& file) const {
        std::ifstream in(file);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) lines.push_back(line);
        return lines;
    }

    fs::path dir_;
};

double expectedInput(SignalType type, int tau) {
    switch (type) {
    case SignalType::Step: return 1.0;
    case SignalType::Impulse: return tau == 0 ? 1.0 : 0.0;
    case SignalType::Harmonic: return std::sin(static_cast<double>(tau));
    }
    return 0.0;
}

} // namespace

// ------------------------------------------------------------ control modes via mock (TEST_P)

class ControlModeTest : public TempDirTest, public ::testing::WithParamInterface<SignalType> {};

TEST_P(ControlModeTest, ModelReceivesExpectedInputAtEveryStep) {
    const SignalType type = GetParam();
    constexpr int kSteps = 12;
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, reset()).Times(1);
    for (int tau = 0; tau < kSteps; ++tau) {
        EXPECT_CALL(model, step(DoubleEq(expectedInput(type, tau)), tau))
            .Times(1)
            .WillOnce(Return(0.5 * tau));
    }
    CoutCapture cout;
    otis::simulate(model, kSteps, type, dir_);
}

INSTANTIATE_TEST_SUITE_P(
    AllControlModes, ControlModeTest,
    ::testing::Values(SignalType::Step, SignalType::Impulse, SignalType::Harmonic),
    [](const ::testing::TestParamInfo<SignalType>& info) {
        return std::string(otis::signalName(info.param));
    });

// ------------------------------------------------------------ plain simulator tests

class SimulatorTest : public TempDirTest {};

TEST_F(SimulatorTest, WritesCsvWithHeaderAndOneRowPerStep) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _)).Times(3).WillRepeatedly(Return(0.5));
    {
        CoutCapture cout;
        otis::simulate(model, 3, SignalType::Step, dir_);
    }
    const auto lines = readLines(dir_ / "step.csv");
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "tau,u,y");
    EXPECT_EQ(lines[1], "0,1.00000000,0.50000000");
    EXPECT_EQ(lines[3], "2,1.00000000,0.50000000");
}

TEST_F(SimulatorTest, FileNameFollowsSignalName) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _)).WillRepeatedly(Return(0.0));
    CoutCapture cout;
    otis::simulate(model, 1, SignalType::Impulse, dir_);
    otis::simulate(model, 1, SignalType::Harmonic, dir_);
    EXPECT_TRUE(fs::exists(dir_ / "impulse.csv"));
    EXPECT_TRUE(fs::exists(dir_ / "harmonic.csv"));
}

TEST_F(SimulatorTest, ConsoleTableMentionsModelAndSignal) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _)).WillRepeatedly(Return(1.0));
    std::string text;
    {
        CoutCapture cout;
        otis::simulate(model, 2, SignalType::Harmonic, dir_);
        text = cout.str();
    }
    EXPECT_NE(text.find("Mock model"), std::string::npos) << text;
    EXPECT_NE(text.find("harmonic"), std::string::npos) << text;
    EXPECT_NE(text.find("CSV saved"), std::string::npos) << text;
}

TEST_F(SimulatorTest, NonFiniteOutputStopsSimulation) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _))
        .WillOnce(Return(1.0))
        .WillOnce(Return(2.0))
        .WillOnce(Return(std::numeric_limits<double>::infinity()));  // exactly 3 calls expected
    std::string text;
    {
        CoutCapture cout;
        otis::simulate(model, 10, SignalType::Step, dir_);
        text = cout.str();
    }
    EXPECT_NE(text.find("non-finite"), std::string::npos) << text;
    EXPECT_EQ(readLines(dir_ / "step.csv").size(), 3u);  // header + 2 finite rows
}

TEST_F(SimulatorTest, NaNOutputAlsoStopsSimulation) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _))
        .WillOnce(Return(std::numeric_limits<double>::quiet_NaN()));
    CoutCapture cout;
    otis::simulate(model, 10, SignalType::Step, dir_);
    EXPECT_EQ(readLines(dir_ / "step.csv").size(), 1u);  // header only
}

TEST_F(SimulatorTest, MissingOutputDirectoryThrows) {
    NiceMock<MockModel> model;
    EXPECT_CALL(model, name()).WillRepeatedly(Return("Mock model"));
    EXPECT_CALL(model, step(_, _)).Times(0);
    EXPECT_THROW(otis::simulate(model, 3, SignalType::Step, dir_ / "no_such_dir"),
                 std::ios_base::failure);
}

TEST_F(SimulatorTest, RealDelayedModelProducesHandCalculatedCsv) {
    otis::DelayedControlModel model(0.5, 2.0, 1);  // 0, 2, 3, 3.5, 3.75 for the step input
    {
        CoutCapture cout;
        otis::simulate(model, 5, SignalType::Step, dir_);
    }
    const auto lines = readLines(dir_ / "step.csv");
    ASSERT_EQ(lines.size(), 6u);
    EXPECT_EQ(lines[1], "0,1.00000000,0.00000000");
    EXPECT_EQ(lines[2], "1,1.00000000,2.00000000");
    EXPECT_EQ(lines[3], "2,1.00000000,3.00000000");
    EXPECT_EQ(lines[4], "3,1.00000000,3.50000000");
    EXPECT_EQ(lines[5], "4,1.00000000,3.75000000");
}

TEST_F(SimulatorTest, SimulationIsRepeatableBecauseOfReset) {
    otis::DelayedControlModel model(0.5, 1.0, 2);
    CoutCapture cout;
    otis::simulate(model, 6, SignalType::Harmonic, dir_);
    const auto first = readLines(dir_ / "harmonic.csv");
    otis::simulate(model, 6, SignalType::Harmonic, dir_);
    EXPECT_EQ(first, readLines(dir_ / "harmonic.csv"));
}

// ------------------------------------------------------------ gnuplot script

TEST_F(SimulatorTest, PlotScriptWithoutResultsContainsHint) {
    {
        CoutCapture cout;
        otis::generatePlotScript(dir_);
    }
    const auto lines = readLines(dir_ / "plot_all.gp");
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(lines.back(), "# Run a simulation first; no CSV files were found.");
}

TEST_F(SimulatorTest, PlotScriptReferencesExistingCsvFilesOnly) {
    fs::create_directories(dir_ / "linear");
    fs::create_directories(dir_ / "nonlinear");
    fs::create_directories(dir_ / "differential");
    std::ofstream(dir_ / "linear" / "step.csv") << "tau,u,y\n";
    std::ofstream(dir_ / "nonlinear" / "impulse.csv") << "tau,u,y\n";
    std::ofstream(dir_ / "differential" / "harmonic.csv") << "tau,u,y\n";
    {
        CoutCapture cout;
        otis::generatePlotScript(dir_);
    }
    std::string script;
    for (const auto& line : readLines(dir_ / "plot_all.gp")) script += line + "\n";
    EXPECT_NE(script.find("linear_step.png"), std::string::npos);
    EXPECT_NE(script.find("nonlinear_impulse.png"), std::string::npos);
    EXPECT_NE(script.find("differential_harmonic.png"), std::string::npos);
    EXPECT_EQ(script.find("linear_harmonic.png"), std::string::npos);
    EXPECT_EQ(script.find("Run a simulation first"), std::string::npos);
}

TEST_F(SimulatorTest, PlotScriptThrowsIfDirectoryIsMissing) {
    CoutCapture cout;
    EXPECT_THROW(otis::generatePlotScript(dir_ / "missing"), std::ios_base::failure);
}
