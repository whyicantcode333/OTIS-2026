#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "models.h"


struct ModelCase {
    int    modelType;  
    double a1;
    double a2;
    double b;
    double dt;
    int    signalType;
    std::vector<double> expected;
};

class AllModelsParamTest : public ::testing::TestWithParam<ModelCase> {};

TEST_P(AllModelsParamTest, MatchesReferenceTrajectory) {
    const auto& p = GetParam();


    switch (p.modelType) {
    case 1: { 
        LinearModel model(p.a1, p.a2, p.b);
        model.reset();
        for (size_t tau = 0; tau < p.expected.size(); ++tau) {
            double u = generateSignal(p.signalType, static_cast<int>(tau));
            double y = model.step(u);
            EXPECT_NEAR(y, p.expected[tau], 1e-4) << "Mismatch at tau=" << tau;
        }
        break;
    }
    case 2: { 
        NonlinearModel model(p.a1, p.b);
        model.reset();
        for (size_t tau = 0; tau < p.expected.size(); ++tau) {
            double u = generateSignal(p.signalType, static_cast<int>(tau));
            double y = model.step(u);
            EXPECT_NEAR(y, p.expected[tau], 1e-4) << "Mismatch at tau=" << tau;
        }
        break;
    }
    case 3: { 
        DifferentialModel model(p.b, p.dt);
        model.reset();
        for (size_t tau = 0; tau < p.expected.size(); ++tau) {
            double u = generateSignal(p.signalType, static_cast<int>(tau));
            double y = model.step(u);
            EXPECT_NEAR(y, p.expected[tau], 1e-4) << "Mismatch at tau=" << tau;
        }
        break;
    }
    default:
        FAIL() << "Unknown model type";
    }
}


INSTANTIATE_TEST_SUITE_P(
    AllModels, AllModelsParamTest,
    ::testing::Values(
        // --- Линейные ---
        ModelCase{ 1, 0.5, 0.2, 1.0, 0.0, 0, {0.0, 1.0, 2.5, 4.45, 6.725} },
        ModelCase{ 1, 0.5, 0.2, 1.0, 0.0, 1, {1.0, 0.5, 0.45, 0.325, 0.2525} },
        ModelCase{ 1, 0.5, 0.2, 1.0, 0.0, 2, {0.0, 0.84147, 1.33003, 0.97443, -0.00358} },

        // --- Нелинейные ---
        ModelCase{ 2, 0.5, 0.0, 0.1, 0.0, 0, {0.0, 0.1, 0.84983, 3.04545} },
        ModelCase{ 2, 0.5, 0.0, 0.1, 0.0, 1, {0.1, 0.04984, 0.02492, 0.01245} },
        ModelCase{ 2, 0.5, 0.0, 0.1, 0.0, 2, {0.0, 0.05958, 0.10494, 0.05255} },

        // --- Дифференциальные ---
        ModelCase{ 3, 0.0, 0.0, 1.0, 0.5, 0, {0.0, 0.5, 1.5, 3.0} },
        ModelCase{ 3, 0.0, 0.0, 1.0, 0.5, 1, {0.5, 0.5, 0.5, 0.5} },
        ModelCase{ 3, 0.0, 0.0, 1.0, 0.5, 2, {0.0, 0.42073, 0.87538, 0.94589} }
    ));