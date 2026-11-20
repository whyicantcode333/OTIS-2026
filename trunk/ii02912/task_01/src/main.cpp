#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "Input.hpp"
#include "Model.hpp"

namespace fs = std::filesystem;

#ifndef PLOT_SCRIPT
#define PLOT_SCRIPT "plot.py"
#endif

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) { std::cout << '\n'; std::exit(0); }
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? "" : s.substr(b, e - b + 1);
}

static std::string fmt(double v) {
    std::ostringstream os; os << v; return os.str();
}

static bool parseDouble(std::string s, double& out) {
    for (char& c : s) {
        if (c == ',') {
            c = '.';
        }
    }
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(v)) {
        return false;
    }
    out = v;
    return true;
}

static bool parseInt(const std::string& s, int lo, int hi, int& out) {
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str() || *end != '\0' || errno == ERANGE || v < lo || v > hi) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

static double readDouble(const std::string& name, double def) {
    while (true) {
        std::string s = readLine("  " + name + " [" + fmt(def) + "]: ");
        if (s.empty()) {
            return def;
        }
        if (double v = 0.0; parseDouble(s, v)) {
            return v;
        }
        std::cout << "  Ошибка: введите число.\n";
    }
}

static int readInt(const std::string& name, int def, int lo, int hi) {
    while (true) {
        std::string s = readLine("  " + name + " [" + std::to_string(def) + "]: ");
        if (s.empty()) {
            return def;
        }
        if (int v = 0; parseInt(s, lo, hi, v)) {
            return v;
        }
        std::cout << "  Ошибка: введите целое число от " << lo << " до " << hi << ".\n";
    }
}

static fs::path simulate(Model& model, const Input& input, int n, const fs::path& outDir) {
    model.reset();
    std::cout << "\n=== " << model.name() << " | input = " << input.name() << " ===\n";
    std::cout << std::setw(6) << "tau" << std::setw(14) << "u_tau" << std::setw(16) << "y_tau" << '\n';
    std::cout << std::string(36, '-') << '\n';

    fs::path file = outDir / (model.name() + "_" + input.name() + ".csv");
    std::ofstream csv(file);
    csv << "tau,u,y\n" << std::fixed << std::setprecision(6);
    std::cout << std::fixed << std::setprecision(6);

    for (int tau = 0; tau <= n; ++tau) {
        double u = input.value(tau);
        double y = model.y();                // y_tau
        std::cout << std::setw(6) << tau << std::setw(14) << u << std::setw(16) << y << '\n';
        csv << tau << ',' << u << ',' << y << '\n';
        model.nextStep(u);                   // -> y_{tau+1}
    }
    return file;
}

static int runPython(const std::string& py, const fs::path& csv) {
    std::string cmd = py + " \"" + PLOT_SCRIPT + "\" \"" + csv.filename().string() + "\"";
#ifdef _WIN32
    cmd = "\"" + cmd + "\"";  
#endif
    return std::system(cmd.c_str());
}

static void showPlot(const fs::path& csv) {
    std::cout << "\nОткрывается график (закройте окно графика, чтобы продолжить)...\n";
#ifdef _WIN32
    if (runPython("python", csv) != 0 && runPython("py", csv) != 0)
#else
    if (runPython("python3", csv) != 0)
#endif
        std::cout << "[!] Не удалось построить график. Установите Python и matplotlib:\n"
                     "    python -m pip install matplotlib\n";
}

int main() {
    std::cout << "Лабораторная работа №1. Моделирование динамических объектов\n";
    const fs::path outDir = "results";
    fs::create_directories(outDir);

    while (true) {
        std::cout << "\nМодель:\n"
                     "  1 - Модель 1.10: y(t+1) = a1*y(t) + b1*u(t) + b2*u(t-2)\n"
                     "  2 - Модель 2.2 : y(t+1) = a*y(t) + b*sat(u(t))\n"
                     "  3 - Модель 3.4 : dy/dt = -a*y^2 (метод Эйлера)\n";
        int m = readInt("Номер модели", 1, 1, 3);

        std::cout << "Параметры модели (Enter - значение по умолчанию):\n";
        std::unique_ptr<Model> model;
        if (m == 1) {
            double a1 = readDouble("a1", 0.7);
            double b1 = readDouble("b1", 0.5);
            double b2 = readDouble("b2", 0.3);
            double y0 = readDouble("y0", 0.0);
            model = std::make_unique<DistributedControlModel>(a1, b1, b2, y0);
        } else if (m == 2) {
            double a    = readDouble("a", 0.8);
            double b    = readDouble("b", 0.5);
            double uMin = readDouble("Umin", -0.5);
            double uMax = readDouble("Umax", 0.5);
            double y0   = readDouble("y0", 0.0);
            if (uMin > uMax) { std::swap(uMin, uMax); std::cout << "  (Umin > Umax: значения поменяны местами)\n"; }
            model = std::make_unique<SaturationModel>(a, b, uMin, uMax, y0);
        } else {
            double a  = readDouble("a", 0.5);
            double y0 = readDouble("y0", 1.0);
            double h  = readDouble("шаг h", 0.1);
            if (h <= 0) { h = 0.1; std::cout << "  (h должен быть > 0, принято h = 0.1)\n"; }
            model = std::make_unique<QuadraticDecayModel>(a, h, y0);
        }

        std::unique_ptr<Input> input = std::make_unique<StepInput>(1.0);
        if (m != 3) {
            std::cout << "Входное воздействие:\n"
                         "  1 - ступенчатое u = const\n"
                         "  2 - импульсное u0 = 1, u>0 = 0\n"
                         "  3 - гармоническое u = sin(tau)\n";
            int t = readInt("Номер воздействия", 1, 1, 3);
            if (t == 1) input = std::make_unique<StepInput>(readDouble("значение const", 1.0));
            else if (t == 2) input = std::make_unique<ImpulseInput>();
            else input = std::make_unique<HarmonicInput>();
        } else {
            std::cout << "(Модель 3.4 автономная: входное воздействие не используется)\n";
        }

        int n = readInt("Количество шагов симуляции n", 20, 1, 100000);

        fs::path csv = simulate(*model, *input, n, outDir);
        std::cout << "\nТаблица сохранена: " << fs::absolute(csv).string() << '\n';
        showPlot(csv);

        std::string again = readLine("\nЗапустить ещё раз? (y/n) [n]: ");
        if (again != "y" && again != "Y" && again != "д" && again != "Д") break;
    }
    return 0;
}
