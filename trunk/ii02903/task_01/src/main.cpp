// Task 01, variant 3: Model 1.3 / Model 2.5 / Model 3.7
// The user chooses ONE model, enters all parameters, the number of steps n and the input signal.
// Results: table in console + separate CSV file for each run (modelXX_<input>.csv, columns t,u,y)
// + automatic plot/table window (Python, plot.py).
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "models.h"
#include "signals.h"

namespace {

struct Row {
    int tau;
    double t;
    double u;
    double y;
};

std::string trim(std::string_view s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string_view::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(b, e - b + 1));
}

// Converts text to a finite real number. Returns false if the text is not a valid number.
bool parseDouble(const std::string& s, double& out) {
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size() || !std::isfinite(v)) return false;
        out = v;
        return true;
    } catch (const std::invalid_argument&) {
        return false;  // not a number at all
    } catch (const std::out_of_range&) {
        return false;  // number is too large
    }
}

// Converts text to an integer from lo to hi. Returns false if the text is not valid.
bool parseInt(const std::string& s, int lo, int hi, int& out) {
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size() || v < lo || v > hi) return false;
        out = v;
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

// Reads a real number. Enter = default value. Both '.' and ',' are accepted.
double readDouble(const std::string& prompt, double def) {
    for (;;) {
        std::cout << prompt << " [" << def << "]: ";
        std::string s;
        if (!std::getline(std::cin, s)) std::exit(0);
        s = trim(s);
        if (s.empty()) return def;
        for (char& c : s) if (c == ',') c = '.';
        if (double v = 0.0; parseDouble(s, v)) return v;
        std::cout << "  Invalid number, try again.\n";
    }
}

int readInt(const std::string& prompt, int def, int lo, int hi) {
    for (;;) {
        std::cout << prompt << " [" << def << "]: ";
        std::string s;
        if (!std::getline(std::cin, s)) std::exit(0);
        s = trim(s);
        if (s.empty()) return def;
        if (int v = 0; parseInt(s, lo, hi, v)) return v;
        std::cout << "  Enter an integer from " << lo << " to " << hi << ".\n";
    }
}

std::string csvNum(double v) {
    if (!std::isfinite(v)) return "nan";
    std::ostringstream os;
    os << std::setprecision(10) << v;
    return os.str();
}

std::unique_ptr<Model> createModel(int choice) {
    std::cout << "\n--- Model parameters ---\n";
    switch (choice) {
        case 1: {
            std::cout << "y(t+1) = a1*y(t) + a2*y(t-1) + b*u(t)\n";
            double a1 = readDouble("a1", 0.9);
            double a2 = readDouble("a2", -0.2);
            double b = readDouble("b", 0.5);
            double y0 = readDouble("initial value y(0)", 0.0);
            return std::make_unique<LinearModel13>(a1, a2, b, y0);
        }
        case 2: {
            std::cout << "y(t+1) = a*y(t) + b*sign(u(t))*(1 - exp(-|u(t)|))\n";
            double a = readDouble("a", 0.8);
            double b = readDouble("b", 1.0);
            double y0 = readDouble("initial value y(0)", 0.0);
            return std::make_unique<NonlinearModel25>(a, b, y0);
        }
        default: {
            std::cout << "dy/dt = -exp(a*y) + b*u   (Euler method)\n";
            double a = readDouble("a", 0.5);
            double b = readDouble("b", 2.0);
            double dt = readDouble("time step dt", 0.1);
            while (dt <= 0.0) dt = readDouble("dt must be > 0, time step dt", 0.1);
            double y0 = readDouble("initial value y(0)", 0.0);
            return std::make_unique<DiffModel37>(a, b, dt, y0);
        }
    }
}

std::unique_ptr<InputSignal> createInput() {
    std::cout << "\n--- Input signal ---\n"
              << "  1 - step      (u = const)\n"
              << "  2 - impulse   (u0 = 1, u>0 = 0)\n"
              << "  3 - harmonic  (u = sin(tau))\n";
    int c = readInt("Choose input signal", 1, 1, 3);
    if (c == 1) return std::make_unique<StepInput>(readDouble("step value u", 1.0));
    if (c == 2) return std::make_unique<ImpulseInput>();
    return std::make_unique<HarmonicInput>();
}

void printRow(const Row& r) {
    std::cout << std::setw(6) << r.tau << std::setw(12) << r.t << std::setw(14) << r.u
              << std::setw(16) << r.y << "\n";
}

void printTable(const std::vector<Row>& rows) {
    std::cout << "\n" << std::setw(6) << "tau" << std::setw(12) << "t" << std::setw(14) << "u"
              << std::setw(16) << "y" << "\n"
              << std::string(48, '-') << "\n";
    std::cout << std::setprecision(6);
    const size_t full = 40;
    if (rows.size() <= full) {
        for (const Row& r : rows) printRow(r);
    } else {
        for (size_t i = 0; i < 20; ++i) printRow(rows[i]);
        std::cout << "   ... (" << rows.size() - 25 << " rows hidden, all of them are in the CSV file)\n";
        for (size_t i = rows.size() - 5; i < rows.size(); ++i) printRow(rows[i]);
    }
}

void saveCsv(const std::string& file, const std::string& title, const std::vector<Row>& rows) {
    std::ofstream f(file);
    f << "# " << title << "\n";
    f << "t,u,y\n";
    for (const Row& r : rows)
        f << csvNum(r.t) << "," << csvNum(r.u) << "," << csvNum(r.y) << "\n";
}

// Starts Python, which shows the plot and the table in a window by itself.
void showPlot(const std::string& csv) {
#ifdef _WIN32
    std::string cmd = "python plot.py " + csv + " || py plot.py " + csv;
#else
    std::string cmd = "python3 plot.py " + csv;
#endif
    std::cout << "\nOpening plot window (close it to continue)...\n";
    int rc = std::system(cmd.c_str());
    if (rc != 0)
        std::cout << "Could not start the plot. Install Python and matplotlib:\n"
                  << "    pip install matplotlib\n"
                  << "(the data is still saved in " << csv << ")\n";
}

}  // namespace

int main() {
    std::cout << std::setprecision(6);
    for (;;) {
        std::cout << "\n==============================================\n"
                  << " Task 01, variant 3 - simulation of models\n"
                  << "==============================================\n"
                  << "  1 - Model 1.3  (linear, 2nd order)\n"
                  << "  2 - Model 2.5  (non-linear, sign * (1 - exp(-|u|)))\n"
                  << "  3 - Model 3.7  (differential equation, Euler method)\n"
                  << "  0 - Exit\n";
        int choice = readInt("Choose model", 1, 0, 3);
        if (choice == 0) break;

        std::unique_ptr<Model> model = createModel(choice);
        std::unique_ptr<InputSignal> input = createInput();
        int n = readInt("\nNumber of simulation steps n", 30, 1, 100000);

        // Stability analysis (advanced level) - before the simulation starts
        std::string info;
        bool stable = model->isStable(info);
        if (!info.empty()) {
            std::cout << "\nStability analysis: " << info << "\n";
            if (!stable)
                std::cout << "!!! WARNING: the system is NOT stable (|z| >= 1) - "
                             "the process will diverge or oscillate without decay !!!\n";
            else
                std::cout << "The system is stable (all |z| < 1).\n";
        }

        const double dt = model->timeStep();
        std::vector<Row> rows;
        rows.reserve(n + 1);
        for (int tau = 0; tau <= n; ++tau) {
            double t = tau * dt;
            double u = input->value(tau, t);
            rows.push_back({tau, t, u, model->current()});
            if (tau < n) model->nextStep(u);
        }

        printTable(rows);

        bool diverged = false;
        for (const Row& r : rows)
            if (!std::isfinite(r.y) || std::fabs(r.y) > 1e6) diverged = true;
        if (diverged)
            std::cout << "\nNote: the values grew without limit (or overflowed) - the process diverges.\n";

        std::string title = model->name() + " | " + model->params() + " | input: " + input->name() +
                            " | n=" + std::to_string(n);
        // separate CSV file for every run, e.g. model13_step.csv
        std::string csvName = model->tag() + "_" + input->tag() + ".csv";
        saveCsv(csvName, title, rows);
        std::cout << "\nResults saved to " << csvName << "\n";

        showPlot(csvName);
    }
    std::cout << "Bye!\n";
    return 0;
}
