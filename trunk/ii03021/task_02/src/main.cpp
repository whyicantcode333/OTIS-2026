#include "models.hpp"
#include "signals.hpp"
#include "simulator.hpp"
#include "stability.hpp"
 
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
 
namespace fs = std::filesystem;
using otis::SignalType;
 
namespace {
 
struct ModelInfo {
    const char* folder;
    const char* title;
};
const std::array kModels{ModelInfo{"linear", "Linear model 1.2 (control delay)"},
                         ModelInfo{"nonlinear", "Nonlinear model 2.4 (cosine + logarithm)"},
                         ModelInfo{"differential", "Differential model 3.6 (Euler method)"}};
 
// Thrown when standard input is closed before a value could be read.
class InputClosedError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
 
// Reads one value from a line; repeats until it is a number inside [lo, hi].
template <typename T>
T ask(const std::string& prompt, T lo, T hi) {
    for (std::string line;;) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) throw InputClosedError("Input stream closed.");
        std::istringstream in(line);
        T value{};
        if (char extra; in >> value && !(in >> extra) && value >= lo && value <= hi) return value;
        std::cout << "Invalid value. Try again.\n";
    }
}
 
// Asks the coefficients of model `id` (0..2), reports its stability and builds it.
std::unique_ptr<otis::Model> buildModel(int id) {
    const double kMax = 1e6;
    std::cout << "\nEnter model coefficients.\n";
    const double a = ask<double>("a: ", -kMax, kMax);
    const double b = ask<double>("b: ", -kMax, kMax);
    std::unique_ptr<otis::Model> model;
    if (id == 0) {
        const int k = ask<int>("Control delay k (>= 1): ", 1, 1000000);
        otis::printStabilityReport(otis::analyzeDelayedControl(a, k), kModels[id].title, std::cout);
        model = std::make_unique<otis::DelayedControlModel>(a, b, k);
    } else if (id == 1) {
        otis::printStabilityReport(otis::analyzeLogarithmic(a), kModels[id].title, std::cout);
        model = std::make_unique<otis::LogarithmicInteractionModel>(a, b);
    } else {
        std::cout << "\nStability analysis is defined for the discrete models 1.2 and 2.4 only.\n";
        model = std::make_unique<otis::CubicDifferentialModel>(a, b, ask<double>("Time step dt (> 0): ", 1e-6, kMax));
    }
    return model;
}
 
int runPlotScript(const fs::path& exeDir, const fs::path& results) {
    const fs::path script = exeDir / "plot.py";
    if (!fs::exists(script)) {
        std::cerr << "Warning: plot.py was not found: " << script.string() << '\n';
        return -1;
    }
#ifdef _WIN32
    const char* python = "python";
#else
    const char* python = "python3";
#endif
    const std::string cmd = std::string(python) + " \"" + script.string() + "\" \"" + results.string() + "\"";
    std::cout << "\nLaunching plot.py...\n" << std::flush;
    return std::system(cmd.c_str());
}
 
} // namespace
 
int main(int argc, char* argv[]) {
    try {
        std::cout << "OTIS Lab 2 - Variant 2: simulator with stability analysis\n\n"
                  << "Models: 1 - linear 1.2, 2 - nonlinear 2.4, 3 - differential 3.6, 4 - all\n";
        const int modelChoice = ask<int>("Model [1-4]: ", 1, 4);
        std::cout << "Signals: 1 - step, 2 - impulse, 3 - harmonic, 4 - all\n";
        const int signalChoice = ask<int>("Input signal [1-4]: ", 1, 4);
        const int steps = argc > 1 ? std::stoi(argv[1]) : ask<int>("Number of steps [1..1000000]: ", 1, 1000000);
        if (steps < 1 || steps > 1000000) throw std::invalid_argument("The step count must be in range 1..1000000.");
 
        const std::vector allSignals = {SignalType::Step, SignalType::Impulse, SignalType::Harmonic};
        std::vector<SignalType> signals = allSignals;
        if (signalChoice < 4) signals = {allSignals[signalChoice - 1]};
 
        const fs::path exeDir = fs::absolute(argv[0]).parent_path();
        const fs::path results = exeDir / "results";
        fs::remove_all(results);
        for (const auto& m : kModels) fs::create_directories(results / m.folder);
 
        for (int id = 0; id < 3; ++id) {
            if (modelChoice != 4 && modelChoice != id + 1) continue;
            std::cout << "\n--------------------------------------------\nSelected: " << kModels[id].title << '\n';
            const auto model = buildModel(id);
            for (const auto signal : signals) otis::simulate(*model, steps, signal, results / kModels[id].folder);
        }
 
        std::cout << "\nSimulation completed. Results: " << results.string() << '\n';
        if (runPlotScript(exeDir, results) != 0) {
            std::cerr << "\nAutomatic plotting failed. Check that Python and matplotlib are installed.\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "\nError: " << error.what() << '\n';
        return 1;
    }
}
