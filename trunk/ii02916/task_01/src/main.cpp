#include <iostream>
#include <memory>
#include <limits>
#include <clocale>
#include <fstream>
#include "Model.h"
#include "Signals.h"
#include "Model1_5.h"
#include "Model2_9.h"
#include "Model3_3.h"

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int getUserChoice() {
    int choice = 0;
    while (true) {
        std::cout << "Выберите модель:\n";
        std::cout << "1. Model 1.5\n";
        std::cout << "2. Model 2.9\n";
        std::cout << "3. Model 3.3\n";
        std::cout << "Ваш выбор (1-3): ";
        if (std::cin >> choice && choice >= 1 && choice <= 3) {
            break;
        }
        std::cout << "Неверный ввод. Пожалуйста, введите число от 1 до 3.\n";
        clearInputBuffer();
    }
    return choice;
}

std::unique_ptr<Model> createModel1_5(int& n) {
    double a1 = 0.0;
    double a2 = 0.0;
    double b = 0.0;
    int k = 0;
    double initialValue = 0.0;

    std::cout << "\n--- Настройка Model 1.5 ---\n";
    std::cout << "Введите коэффициент a1 (по умолчанию 1.0): ";
    while (!(std::cin >> a1)) { clearInputBuffer(); }
    std::cout << "Введите коэффициент a2 (по умолчанию 0.5): ";
    while (!(std::cin >> a2)) { clearInputBuffer(); }
    std::cout << "Введите коэффициент b (по умолчанию 1.0): ";
    while (!(std::cin >> b)) { clearInputBuffer(); }
    std::cout << "Введите задержку k (по умолчанию 1): ";
    while (!(std::cin >> k)) { clearInputBuffer(); }
    std::cout << "Введите начальное значение x0 (по умолчанию 0.0): ";
    while (!(std::cin >> initialValue)) { clearInputBuffer(); }
    std::cout << "Введите количество шагов n: ";
    while (!(std::cin >> n) || n <= 0) { clearInputBuffer(); }

    return std::make_unique<Model1_5>(a1, a2, b, k, initialValue);
}

std::unique_ptr<Model> createModel2_9(int& n) {
    double initialValue = 0.0;
    double a = 0.0;
    double b = 0.0;

    std::cout << "\n--- Настройка Model 2.9 (Square Root Modulated Action) ---\n";
    std::cout << "Введите начальное значение x0 (по умолчанию 0.0): ";
    while (!(std::cin >> initialValue)) { clearInputBuffer(); }
    std::cout << "Введите коэффициент a (по умолчанию 1.0): ";
    while (!(std::cin >> a)) { clearInputBuffer(); }
    std::cout << "Введите коэффициент b (по умолчанию 1.0): ";
    while (!(std::cin >> b)) { clearInputBuffer(); }
    std::cout << "Введите количество шагов n: ";
    while (!(std::cin >> n) || n <= 0) { clearInputBuffer(); }

    return std::make_unique<Model2_9>(initialValue, a, b);
}

std::unique_ptr<Model> createModel3_3(int& n) {
    double initialValue = 0.0;
    double tau = 0.0;
    double gain = 0.0;

    std::cout << "\n--- Настройка Model 3.3 (Standard First-Order Process) ---\n";
    std::cout << "Введите начальное значение y0 (по умолчанию 0.0): ";
    while (!(std::cin >> initialValue)) { clearInputBuffer(); }
    std::cout << "Введите постоянную времени tau (по умолчанию 1.0): ";
    while (!(std::cin >> tau)) { clearInputBuffer(); }
    std::cout << "Введите коэффициент усиления gain (по умолчанию 1.0): ";
    while (!(std::cin >> gain)) { clearInputBuffer(); }
    std::cout << "Введите количество шагов n: ";
    while (!(std::cin >> n) || n <= 0) { clearInputBuffer(); }

    return std::make_unique<Model3_3>(initialValue, tau, gain);
}

std::unique_ptr<Model> createModel(int choice, int& n) {
    switch (choice) {
    case 1: return createModel1_5(n);
    case 2: return createModel2_9(n);
    case 3: return createModel3_3(n);
    default: return nullptr;
    }
}

SignalType getSignalChoice(double& amplitude) {
    int sigChoice = 1;
    std::cout << "\nВыберите тип входного сигнала:\n";
    std::cout << "1. Ступенчатый (Step, u = const)\n";
    std::cout << "2. Импульсный (Impulse)\n";
    std::cout << "3. Гармонический (Harmonic, синусоида)\n";
    std::cout << "Ваш выбор (1-3): ";

    while (true) {
        if (std::cin >> sigChoice && sigChoice >= 1 && sigChoice <= 3) {
            break;
        }
        std::cout << "Неверный ввод. Введите число от 1 до 3: ";
        clearInputBuffer();
    }

    std::cout << "Введите амплитуду сигнала (по умолчанию 1.0): ";
    if (!(std::cin >> amplitude)) {
        clearInputBuffer();
        amplitude = 1.0;
    }

    switch (sigChoice) {
    case 1: return SignalType::Step;
    case 2: return SignalType::Impulse;
    case 3: return SignalType::Harmonic;
    default: return SignalType::Step;
    }
}

int main() {
    std::setlocale(LC_ALL, "");
    int n = 0;
    int choice = getUserChoice();
    std::unique_ptr<Model> model = createModel(choice, n);

    if (model) {
        std::cout << "Модель успешно создана: " << model->getName() << "\n";
    }
    std::ofstream outFile("result.csv");
    if (!outFile.is_open()) {
        std::cerr << "[Ошибка] Не удалось открыть файл result.csv для записи!\n";
        return 1;
    }

    outFile << "Step,Input,Output\n";

    double amplitude = 1.0;
    SignalType sigType = getSignalChoice(amplitude);

    double currentOutput = 0.0;
    std::cout << "Результат вычислений\n";
    std::cout << "Step\t Input\t Output\n";
    for (int step = 0; step < n; ++step) {
        double u = getInput(sigType, step, amplitude);
        currentOutput = model->calculateNext(u, 0.1);
        outFile << step << "," << u << "," << currentOutput << "\n";
        std::cout << step << "\t " << u << "\t " << currentOutput << "\n";
    }

    outFile.close();
    std::cout << "Симуляция завершена. Данные записаны в result.csv.\n";

    return 0;
}
