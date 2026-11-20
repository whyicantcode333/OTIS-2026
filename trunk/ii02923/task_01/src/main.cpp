#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include "InputSignal.h"
#include "Model.h"
#include "Model1_6.h"
#include "Model2_1.h"
#include "Model3_7.h"

int validateStepLimit(int raw_steps) {
    constexpr int lower_bound = 1;
    constexpr int upper_bound = 1000;
    constexpr int fallback_val = 30;

    if (raw_steps < lower_bound || raw_steps > upper_bound) {
        return fallback_val;
    }
    return raw_steps;
}

double fetchFloatingParameter(const std::string& msg, double default_val) {
    std::cout << msg << " [default: " << default_val << "]: ";
    if (std::string line; std::getline(std::cin, line) && !line.empty()) {
        std::stringstream stream(line);
        double parsed_val = 0.0;
        if (stream >> parsed_val) {
            return parsed_val;
        }
    }
    return default_val;
}

int querySystemSelection() {
    std::cout << "Choose mathematical model configuration:\n"
              << "  [1] Model 1.6 (Third-Order: y[t+1] = a1*y[t] + a2*y[t-1] + a3*y[t-2] + b*u[t])\n"
              << "  [2] Model 2.1 (Nonlinear: y[t+1] = a*y[t] - b*y[t-1]^2 + c*u[t] + d*sin(u[t-1]))\n"
              << "  [3] Model 3.7 (ODE: dy/dt = -exp(a)*y + b*u)\n"
              << "Select model index (1-3, default: 1): ";
    if (std::string line; std::getline(std::cin, line) && !line.empty()) {
        std::stringstream stream(line);
        int option = 1;
        if (stream >> option && (option >= 1 && option <= 3)) {
            return option;
        }
    }
    return 1;
}

SignalType querySignalSelection() {
    std::cout << "\nSpecify driving signal type:\n"
              << "  [1] Step (u = A)\n"
              << "  [2] Impulse (u(0) = A, u(t>0) = 0)\n"
              << "  [3] Harmonic (u = A * sin(t))\n"
              << "Select signal option (1-3, default: 1): ";
    if (std::string line; std::getline(std::cin, line) && !line.empty()) {
        std::stringstream stream(line);
        int option = 1;
        if (stream >> option) {
            switch (option) {
                case 2:
                    return SignalType::IMPULSE;
                case 3:
                    return SignalType::HARMONIC;
                case 1:
                default:
                    return SignalType::STEP;
            }
        }
    }
    return SignalType::STEP;
}

int main(int argc, char* argv[]) {
    int active_steps = 30;

    if (argc > 1) {
        try {
            active_steps = validateStepLimit(std::stoi(argv[1]));
        } catch (...) {
            active_steps = 30;
        }
    } else {
        std::cout << "Specify total execution steps (default: 30): ";
        if (std::string line; std::getline(std::cin, line) && !line.empty()) {
            std::stringstream stream(line);
            int val = 0;
            if (stream >> val) {
                active_steps = validateStepLimit(val);
            }
        }
    }

    const int steps_count = active_steps;
    const int selected_model_id = querySystemSelection();
    const SignalType chosen_signal = querySignalSelection();

    std::unique_ptr<Model> target_model;
    std::string active_model_label;

    std::cout << "\n=== Configuring Model Parameters ===\n";

    switch (selected_model_id) {
        case 2: {
            const double param_a = fetchFloatingParameter("Value for coefficient a", 0.5);
            const double param_b = fetchFloatingParameter("Value for coefficient b", 0.1);
            const double param_c = fetchFloatingParameter("Value for coefficient c", 0.4);
            const double param_d = fetchFloatingParameter("Value for coefficient d", 0.2);
            const double init_y = fetchFloatingParameter("Initial state y0", 0.0);
            target_model = std::make_unique<Model2_1>(param_a, param_b, param_c, param_d, init_y);
            active_model_label = "Model 2.1";
            break;
        }
        case 3: {
            const double param_a = fetchFloatingParameter("Value for coefficient a", 0.2);
            const double param_b = fetchFloatingParameter("Value for coefficient b", 0.5);
            const double step_dt = fetchFloatingParameter("Time step increment dt", 0.1);
            const double init_y = fetchFloatingParameter("Initial state y0", 0.0);
            target_model = std::make_unique<Model3_7>(param_a, param_b, step_dt, init_y);
            active_model_label = "Model 3.7";
            break;
        }
        case 1:
        default: {
            const double param_a1 = fetchFloatingParameter("Value for coefficient a1", 0.5);
            const double param_a2 = fetchFloatingParameter("Value for coefficient a2", 0.2);
            const double param_a3 = fetchFloatingParameter("Value for coefficient a3", 0.1);
            const double param_b  = fetchFloatingParameter("Value for coefficient b", 0.4);
            const double init_y   = fetchFloatingParameter("Initial state y0", 0.0);
            target_model = std::make_unique<Model1_6>(param_a1, param_a2, param_a3, param_b, init_y);
            active_model_label = "Model 1.6";
            break;
        }
    }

    const std::string current_signal_name = InputSignal::getSignalName(chosen_signal);

    std::ofstream csv_stream("results.csv");
    if (!csv_stream.is_open()) {
        std::cerr << "Error: Could not establish output file stream results.csv\n";
        return 1;
    }

    csv_stream << "step,signal,u,y,model_name\n";

    std::cout << "\n--------------------------------------------\n";
    std::cout << "Execution initiated for the chosen configuration\n";
    std::cout << "--------------------------------------------\n";

    std::cout << std::setw(6) << "Step"
              << std::setw(12) << "Input u"
              << std::setw(16) << "Output y" << "\n";

    for (int idx = 0; idx < steps_count; ++idx) {
        const double u_val = InputSignal::getSignal(chosen_signal, idx);
        const double y_val = target_model->step(u_val);

        std::cout << std::setw(6) << idx
                  << std::fixed << std::setprecision(4)
                  << std::setw(12) << u_val
                  << std::setw(16) << y_val << "\n";

        csv_stream << idx << "," << current_signal_name << "," << u_val << ","
                   << y_val << "," << active_model_label << "\n";
    }

    csv_stream.close();
    std::cout << "\nData successfully serialized into results.csv\n";
    return 0;
}