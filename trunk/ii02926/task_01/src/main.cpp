#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>

#include "Model.h"
#include "Model1_7.h"
#include "Model2_9.h"
#include "Model3_1.h"
#include "InputSignals.h"

namespace {
    constexpr double DEFAULT_A = 0.5;
    constexpr double DEFAULT_B1 = 0.4;
    constexpr double DEFAULT_B2 = 0.2;
    constexpr double DEFAULT_B3 = 0.1;
    constexpr double DEFAULT_B = 0.5;
    constexpr double DEFAULT_DT = 0.05;
    constexpr int    DEFAULT_N = 20;
    constexpr int    CSV_PRECISION = 5;
}

static double readDouble(const std::string& prompt, double def) {
    std::cout << prompt << " [default " << def << "]: ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return def;
    try {
        return std::stod(line);
    } catch (const std::invalid_argument&) {
        std::cout << "  Failed to parse, using default value.\n";
        return def;
    } catch (const std::out_of_range&) {
        std::cout << "  Value out of range, using default value.\n";
        return def;
    }
}

static int readInt(const std::string& prompt, int def) {
    std::cout << prompt << " [default " << def << "]: ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return def;
    try {
        return std::stoi(line);
    } catch (const std::invalid_argument&) {
        std::cout << "  Failed to parse, using default value.\n";
        return def;
    } catch (const std::out_of_range&) {
        std::cout << "  Value out of range, using default value.\n";
        return def;
    }
}

static int readChoice(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        try {
            int v = std::stoi(line);
            if (v >= lo && v <= hi) return v;
        } catch (const std::invalid_argument&) {
            std::cout << "  Invalid input. Please enter a valid number.\n";
            continue;
        } catch (const std::out_of_range&) {
            std::cout << "  Number out of range. Please try again.\n";
            continue;
        }
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

int main() {
    std::cout << "===========================================\n";
    std::cout << " Lab #1 (OTIS) - ii02926\n";
    std::cout << " Modeling of a controlled object\n";
    std::cout << "===========================================\n\n";

    std::cout << "Choose a model:\n";
    std::cout << "  1) Model 1.7 - Linear (Multi-Step Control History)\n";
    std::cout << "  2) Model 2.9 - Non-linear (Square Root Modulated Action)\n";
    std::cout << "  3) Model 3.1 - Diff. equation (Pure Linear Decay)\n";
    const int modelChoice = readChoice("Your choice (1-3): ", 1, 3);
    std::cout << "\n";

    std::unique_ptr<Model> model;
    if (modelChoice == 1) {
        const double a  = readDouble("Enter coefficient a", DEFAULT_A);
        const double b1 = readDouble("Enter coefficient b1", DEFAULT_B1);
        const double b2 = readDouble("Enter coefficient b2", DEFAULT_B2);
        const double b3 = readDouble("Enter coefficient b3", DEFAULT_B3);
        model = std::make_unique<Model1_7>(a, b1, b2, b3);
    } else if (modelChoice == 2) {
        const double a = readDouble("Enter coefficient a", 0.6);
        const double b = readDouble("Enter coefficient b", DEFAULT_B);
        model = std::make_unique<Model2_9>(a, b);
    } else {
        const double a  = readDouble("Enter coefficient a", 0.8);
        const double dt = readDouble("Enter time step dt", DEFAULT_DT);
        model = std::make_unique<Model3_1>(a, dt);
    }

    std::cout << "\nModel: " << model->name() << "\n";

    if (!model->isStable()) {
        if (const std::string warn = model->stabilityWarning(); !warn.empty()) {
            std::cout << "\n!!! WARNING !!!\n" << warn << "\n";
        }
        std::cout << "Continue simulation? (y/n): ";
        std::string ans;
        std::getline(std::cin, ans);
        if (!(ans == "y" || ans == "Y")) {
            std::cout << "Cancelled by user.\n";
            return 0;
        }
    }

    std::cout << "\nChoose input signal:\n";
    std::cout << "  1) Step\n";
    std::cout << "  2) Impulse\n";
    std::cout << "  3) Harmonic\n";
    const int sigChoice = readChoice("Your choice (1-3): ", 1, 3);

    std::unique_ptr<InputSignal> signal;
    switch (sigChoice) {
        case 1: signal = std::make_unique<StepInput>(); break;
        case 2: signal = std::make_unique<ImpulseInput>(); break;
        case 3: signal = std::make_unique<HarmonicInput>(); break;
        default:
            std::cout << "Invalid signal choice.\n";
            return 1;
    }
    std::cout << "Signal: " << signal->name() << "\n";

    const int n = readInt("\nEnter number of simulation steps n", DEFAULT_N);
    if (n <= 0) {
        std::cout << "n must be > 0\n";
        return 1;
    }

    std::cout << "\n===========================================\n";
    std::cout << std::setw(6) << "tau"
              << std::setw(14) << "u_tau"
              << std::setw(14) << "y_tau" << "\n";
    std::cout << "-------------------------------------------\n";

    std::ofstream csv("result.csv");
    csv << "tau,u_tau,y_tau\n";

    model->reset();
    for (int tau = 1; tau <= n; ++tau) {
        const double u = signal->value(tau);
        const double y = model->next(u);

        std::cout << std::setw(6) << tau
                  << std::setw(14) << std::fixed << std::setprecision(CSV_PRECISION) << u
                  << std::setw(14) << std::fixed << std::setprecision(CSV_PRECISION) << y
                  << "\n";

        csv << tau << "," << u << "," << y << "\n";
    }

    csv.close();
    std::cout << "-------------------------------------------\n";
    std::cout << "Results saved to result.csv\n";
    std::cout << "Done.\n";

    return 0;
}
