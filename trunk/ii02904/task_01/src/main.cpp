#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "models.h"

const double MAX_Y = 1e9;
const char* const RESULT_FILE = "results.csv";

const int TAU_WIDTH = 6;
const int VALUE_WIDTH = 18;

void printLine()
{
    std::cout << "+" << std::string(TAU_WIDTH + 2, '-') << "+" << std::string(VALUE_WIDTH + 2, '-')
              << "+" << std::string(VALUE_WIDTH + 2, '-') << "+\n";
}

void printHeader()
{
    printLine();
    std::cout << "| " << std::setw(TAU_WIDTH) << "tau" << " | " << std::setw(VALUE_WIDTH) << "u"
              << " | " << std::setw(VALUE_WIDTH) << "y" << " |\n";
    printLine();
}

void printRow(int tau, double u, double y)
{
    std::cout << "| " << std::setw(TAU_WIDTH) << tau << " | " << std::setw(VALUE_WIDTH) << u
              << " | " << std::setw(VALUE_WIDTH) << y << " |\n";
}

double readNumber(const std::string& text)
{
    while (true) {
        std::cout << text;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nВвод закончился, выход.\n";
            std::exit(1);
        }
        std::replace(line.begin(), line.end(), ',', '.');
        try {
            return std::stod(line);
        } catch (const std::invalid_argument&) {
            std::cout << "Нужно ввести число, попробуйте ещё раз.\n";
        } catch (const std::out_of_range&) {
            std::cout << "Нужно ввести число, попробуйте ещё раз.\n";
        }
    }
}

int readInt(const std::string& text, int minValue, int maxValue)
{
    while (true) {
        if (const auto value = static_cast<int>(readNumber(text)); value >= minValue && value <= maxValue) {
            return value;
        }
        std::cout << "Число должно быть от " << minValue << " до " << maxValue << ".\n";
    }
}

std::unique_ptr<Model> createModel(int choice, double y0)
{
    if (choice == 1) {
        double a = readNumber("a = ");
        double b = readNumber("b = ");
        if (std::abs(a) >= 1) {
            std::cout << "\nПРЕДУПРЕЖДЕНИЕ: |a| >= 1, система неустойчива, y будет расти!\n";
        }
        return std::make_unique<Model14>(a, b, y0);
    }

    if (choice == 2) {
        double a1 = readNumber("a1 = ");
        double a2 = readNumber("a2 = ");
        double b = readNumber("b = ");
        return std::make_unique<Model26>(a1, a2, b, y0);
    }

    double a1 = readNumber("a1 = ");
    double a2 = readNumber("a2 = ");
    double b = readNumber("b = ");
    double dt = readNumber("Шаг dt = ");
    return std::make_unique<Model38>(a1, a2, b, dt, y0);
}

double getInput(int signalType, int tau, double stepValue)
{
    if (signalType == 1) {
        return stepValue;
    }
    if (signalType == 2) {
        return tau == 0 ? 1.0 : 0.0;
    }
    return std::sin(static_cast<double>(tau));
}

std::string signalName(int signalType)
{
    if (signalType == 1) {
        return "ступенчатый сигнал";
    }
    if (signalType == 2) {
        return "импульсный сигнал";
    }
    return "гармонический сигнал";
}

int main()
{
    std::cout << "Лабораторная работа 1, вариант 4\n\n";

    std::cout << "Выберите модель:\n"
              << "  1 - Модель 1.4 (линейная)\n"
              << "  2 - Модель 2.6 (нелинейная)\n"
              << "  3 - Модель 3.8 (дифференциальное уравнение)\n";
    int modelChoice = readInt("Ваш выбор: ", 1, 3);
    double y0 = readNumber("Начальное значение y(0) = ");
    std::unique_ptr<Model> model = createModel(modelChoice, y0);

    std::cout << "\nВыберите входной сигнал:\n"
              << "  1 - ступенчатый (u = const)\n"
              << "  2 - импульсный (u(0) = 1, дальше 0)\n"
              << "  3 - гармонический (u = sin(t))\n";
    int signalType = readInt("Ваш выбор: ", 1, 3);
    double stepValue = 0.0;
    if (signalType == 1) {
        stepValue = readNumber("Значение u = ");
    }

    int n = readInt("\nЧисло шагов n (от 1 до 10000): ", 1, 10000);

    std::ofstream file(RESULT_FILE);
    if (!file) {
        std::cout << "Не удалось создать файл " << RESULT_FILE << "\n";
        return 1;
    }
    file << model->name() << ", " << signalName(signalType) << "\n";
    file << "tau,u,y\n";

    file << std::setprecision(10);
    std::cout << "\n" << std::fixed << std::setprecision(6);
    printHeader();

    double y = y0;
    bool diverged = false;
    int divergedStep = 0;
    for (int tau = 0; tau <= n; tau++) {
        double u = getInput(signalType, tau, stepValue);

        printRow(tau, u, y);
        file << tau << "," << u << "," << y << "\n";

        if (tau < n) {
            y = model->nextStep(u);
            if (!std::isfinite(y) || std::abs(y) > MAX_Y) {
                diverged = true;
                divergedStep = tau + 1;
                break;
            }
        }
    }
    printLine();

    if (diverged) {
        std::cout << "\nПРЕДУПРЕЖДЕНИЕ: процесс расходится (шаг " << divergedStep
                  << "), расчёт остановлен.\n";
    }
    std::cout << "\nРезультаты сохранены в файл " << RESULT_FILE << "\n";
    return 0;
}
