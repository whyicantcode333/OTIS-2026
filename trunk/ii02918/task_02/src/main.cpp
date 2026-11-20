#include <iostream>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <cstdlib>

#include "DynamicModel.h"
#include "SignalGenerator.h"
#include "ModelCreator.h"

namespace {
    constexpr int    STEPS_DEFAULT = 20;
    constexpr double LEVEL_DEFAULT = 1.0;
    constexpr int    PRECISION_CSV = 5;
}

static int readInteger(const std::string& text, int minVal)
{
    int val;
    bool ok = false;
    while (!ok)
    {
        std::cout << text;
        if (std::cin >> val && val >= minVal)
        {
            ok = true;
        }
        else
        {
            std::cout << "  Input error. Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }
    }
    return val;
}

static std::unique_ptr<SignalGenerator> makeSignal(int id)
{
    if (id == 1) return std::make_unique<StepSignal>(LEVEL_DEFAULT);
    if (id == 2) return std::make_unique<BurstSignal>();
    if (id == 3) return std::make_unique<HarmonicSignal>();
    return nullptr;
}

int main()
{
    std::cout << "==============================\n";
    std::cout << " OTIS-2026 | Lab #2 | Var 28\n";
    std::cout << " Student: ii02918\n";
    std::cout << "==============================\n\n";

    std::cout << "Choose model:\n";
    std::cout << "  [1] Model 1.5 (State Delay)\n";
    std::cout << "  [2] Model 2.2 (Saturation)\n";
    std::cout << "  [3] Model 3.8 (Quadratic Decay)\n";
    const int m_id = readInteger("Enter id (1-3): ", 1);
    
    if (m_id > 3)
    {
        std::cerr << "Error: unknown model.\n";
        return 1;
    }

    std::cout << "\nParameters for model:\n";
    std::unique_ptr<DynamicModel> mdl = ModelCreator::create(m_id);
    
    if (!mdl)
    {
        std::cerr << "Creation failed.\n";
        return 1;
    }
    
    std::cout << "\nSelected: " << mdl->describe() << "\n";

    if (!mdl->verifyStability())
    {
        if (const std::string msg = mdl->getWarningMessage(); !msg.empty())
        {
            std::cout << "\n[ALERT] " << msg << "\n";
        }
        std::cout << "Proceed? (y/n): ";
        std::string ans;
        std::cin >> ans;
        if (ans != "y" && ans != "Y")
        {
            std::cout << "Cancelled.\n";
            return 0;
        }
    }

    std::cout << "\nChoose signal:\n";
    std::cout << "  [1] Step\n";
    std::cout << "  [2] Burst\n";
    std::cout << "  [3] Harmonic\n";
    const int s_id = readInteger("Enter id (1-3): ", 1);

    std::unique_ptr<SignalGenerator> sig = makeSignal(s_id);
    if (!sig)
    {
        std::cerr << "Error: bad signal.\n";
        return 1;
    }
    std::cout << "Using: " << sig->getName() << "\n";

    const int total_steps = readInteger("\nIterations count: ", 1);

    std::cout << "\n---------------- SIMULATION START ----------------\n";
    std::cout << std::setw(6) << "Step"
              << std::setw(14) << "Input"
              << std::setw(14) << "Output" << "\n";
    std::cout << "--------------------------------------------------\n";

    std::ofstream file("result.csv");
    file << "tau,u_tau,y_tau\n";

    mdl->reset();
    for (int i = 1; i <= total_steps; ++i)
    {
        const double in_val = sig->value(i);
        const double out_val = mdl->advance(in_val);

        std::cout << std::setw(6) << i
                  << std::setw(14) << std::fixed << std::setprecision(PRECISION_CSV) << in_val
                  << std::setw(14) << std::fixed << std::setprecision(PRECISION_CSV) << out_val
                  << "\n";

        file << i << "," << in_val << "," << out_val << "\n";
    }
    file.close();

    std::cout << "--------------------------------------------------\n";
    std::cout << "Data saved to result.csv\n";

    std::cout << "Launching plotting script...\n";
    const std::string cmd = "python plot_results.py result.csv result_plot.png \"Lab 2 Result\"";
    
    if (const int res = std::system(cmd.c_str()); res == 0) // NOSONAR
    {
        std::cout << "[OK] Plot generated.\n";
        std::system("start \"\" result_plot.png"); // NOSONAR
    }
    else
    {
        std::cout << "[WARN] Plotting failed. Check python.\n";
    }

    std::cout << "Finished.\n";
    return 0;
}