#include <iostream>
#include <memory>
#include <vector>
#include <limits>
#include <cmath>
#include <cstdlib>

#include "Model1_10.h"
#include "Model2_2.h"
#include "Model3_4.h"
#include "StabilityAnalyzer.h"
#include "Simulation.h"

namespace {

template <typename T>
T promptValue(const std::string& prompt) {
    T val{};
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && std::isfinite(static_cast<double>(val))) {
            return val;
        }
        std::cout << "Invalid value. Please re-enter.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::unique_ptr<Model> buildSelectedModel(int choice, double& outInitialY) {
    outInitialY = 0.0;
    if (choice == 1) {
        const double a1 = promptValue<double>("a1: ");
        const double b1 = promptValue<double>("b1: ");
        const double b2 = promptValue<double>("b2: ");

        if (!StabilityAnalyzer::isFirstOrderLinearStable(a1)) {
            std::cout << "\n" << StabilityAnalyzer::getWarningMessage(a1) << "\n\n";
        }
        return std::make_unique<Model1_10>(a1, b1, b2);
    }
    if (choice == 2) {
        const double a = promptValue<double>("a: ");
        const double b = promptValue<double>("b: ");
        double uMin = promptValue<double>("uMin: ");
        double uMax = promptValue<double>("uMax: ");
        while (uMin > uMax) {
            std::cout << "uMin must be <= uMax.\n";
            uMin = promptValue<double>("uMin: ");
            uMax = promptValue<double>("uMax: ");
        }
        return std::make_unique<Model2_2>(a, b, uMin, uMax);
    }
    const double coeff = promptValue<double>("coefficient a: ");
    outInitialY = 2.0;
    return std::make_unique<Model3_4>(coeff);
}

} // namespace

int main() {
    std::cout << "=== Task 02: System Simulation & Stability Check ===\n"
              << "1 - Model1_10 (Linear)\n"
              << "2 - Model2_2 (Saturation)\n"
              << "3 - Model3_4 (Differential)\n";

    int modelId = promptValue<int>("Select model [1-3]: ");
    while (modelId < 1 || modelId > 3) {
        modelId = promptValue<int>("Select model [1-3]: ");
    }

    double initialY = 0.0;
    auto model = buildSelectedModel(modelId, initialY);

    int steps = promptValue<int>("Steps [1-100000]: ");
    while (steps < 1 || steps > 100000) {
        steps = promptValue<int>("Steps [1-100000]: ");
    }

    const double amplitude = promptValue<double>("Amplitude: ");
    int sigId = promptValue<int>("Signal (1=Constant, 2=Impulse, 3=Harmonic): ");
    while (sigId < 1 || sigId > 3) {
        sigId = promptValue<int>("Signal (1=Constant, 2=Impulse, 3=Harmonic): ");
    }

    double omega = 1.0;
    InputSignalType sigType = InputSignalType::Constant;
    if (sigId == 2) {
        sigType = InputSignalType::Impulse;
    } else if (sigId == 3) {
        sigType = InputSignalType::Harmonic;
        omega = promptValue<double>("Angular frequency (>0): ");
        while (omega <= 0.0) {
            omega = promptValue<double>("Angular frequency (>0): ");
        }
    }

    const double dt = 0.1;
    StandardInputSignalProvider provider(sigType, amplitude, omega);

    std::vector<double> u(static_cast<std::size_t>(steps) + 1);
    for (int t = 0; t <= steps; ++t) {
        u[static_cast<std::size_t>(t)] = provider.getSignal(static_cast<std::size_t>(t), t * dt);
    }

    if (const auto y = runSimulation(*model, u, initialY, dt);
        exportSimulationToHtml("simulation_report.html", *model, u, y, dt)) {
        std::cout << "Report generated: simulation_report.html\n";
#ifdef _WIN32
        std::system("start \"\" \"simulation_report.html\"");
#endif
    }
    return 0;
}