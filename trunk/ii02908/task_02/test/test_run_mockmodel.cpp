#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "models.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

// ---------- Mock-модель ----------
class MockModel : public Model {
public:
    MOCK_METHOD(double, step, (double u), (override));
    MOCK_METHOD(void,   reset, (),         (override));
};

// -------- Проверка runSimulation --------
//Проверка того, что reset() будет вызван ровно один раз, а step() - N(5) раз
TEST(RunSimulationTest, CallsResetAndStepsNTimes) {
    MockModel mock;

    EXPECT_CALL(mock, reset()).Times(1);
    EXPECT_CALL(mock, step(_)).Times(5).WillRepeatedly(Return(0.0));

    runSimulation(&mock, 5, /*signal type=*/0, "mock_out.csv");
}

//Проверка правильности посылаемых ступенчатых сигналов
TEST(RunSimulationTest, PassesCorrectSignalsToStep) {
    MockModel mock;

    // Для step-сигнала u(tau)=tau
    InSequence seq;
    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(0.0));
    EXPECT_CALL(mock, step(1.0));
    EXPECT_CALL(mock, step(2.0));
    EXPECT_CALL(mock, step(3.0));

    runSimulation(&mock, 4, /*signal type=*/0, "mock_out2.csv");
}

//Проверка правильности посылаемых импульсных сигналов
TEST(RunSimulationTest, PassesImpulseSignalCorrectly) {
    MockModel mock;

    InSequence seq;
    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(1.0)); //tau=0
    EXPECT_CALL(mock, step(0.0)); //tau=1
    EXPECT_CALL(mock, step(0.0)); //tau=2

    runSimulation(&mock, 3, /*signal ензу=*/1, "mock_out3.csv");
}

//Проверка корректности создаваемого .csv файла
//(создаётся ли файл, открывается ли он, пишется ли туда информация, сколько строк и столбцов)
TEST(RunSimulationTest, WritesCsvFile) {
    MockModel mock;
    EXPECT_CALL(mock, reset());
    EXPECT_CALL(mock, step(_)).WillRepeatedly(Return(42.0));

    const std::string fname = "mock_csv_test.csv";
    runSimulation(&mock, 3, 0, fname);

    std::ifstream fin(fname);
    ASSERT_TRUE(fin.is_open());

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(fin, line)) lines.push_back(line);

    ASSERT_EQ(lines.size(), 3u); // ровно n строк

    // строка формата "tau,u,y"
    std::stringstream ss(lines[0]);
    std::string field;
    std::vector<std::string> fields;
    while (std::getline(ss, field, ',')) fields.push_back(field);
    ASSERT_EQ(fields.size(), 3u);
    EXPECT_EQ(fields[0], "0");
    EXPECT_DOUBLE_EQ(std::stod(fields[2]), 42.0);

    fin.close();
    std::remove(fname.c_str());
}