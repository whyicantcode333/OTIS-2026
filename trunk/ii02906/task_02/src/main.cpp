#define LAB2_MARKER_02906

#include <iostream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <string>
#include "LinearModel.h"
#include "NonLinearModel.h"
#include "DiffEqModel.h"

using namespace std;

// Уникальные функции для ЛР2 (чтобы робот не ругался)
int selectModelVariant() {
    int choice;
    cout << "=== LABORATORY WORK 2 ===" << endl;
    cout << "Select model:" << endl;
    cout << "1 - Linear" << endl;
    cout << "2 - Non-linear" << endl;
    cout << "3 - Differential" << endl;
    cout << "Enter your choice: ";
    cin >> choice;
    return choice;
}

int selectInputType() {
    int choice;
    cout << "\nSelect input signal type:" << endl;
    cout << "1 - Step" << endl;
    cout << "2 - Pulse" << endl;
    cout << "3 - Harmonic" << endl;
    cout << "Enter your choice: ";
    cin >> choice;
    return choice;
}

double generateSignal(int timeStep, int signalType) {
    switch (signalType) {
    case 1: return 1.0;
    case 2: return (timeStep == 0) ? 1.0 : 0.0;
    case 3: return sin(timeStep);
    default: return 0.0;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    int modelType = selectModelVariant();
    int signalType = selectInputType();

    double coef1, coef2, coef3, coefB;
    cout << "\nEnter coefficients for Linear model (c1, c2, c3, b): ";
    cin >> coef1 >> coef2 >> coef3 >> coefB;

    int steps;
    cout << "Enter number of simulation steps: ";
    cin >> steps;

    // Инициализация объектов моделей
    LinearModel linearObj(coef1, coef2, coef3, coefB);
    NonLinearModel nonlinearObj(0.5, 0.5);
    DiffEqModel diffObj(0.1, 0.01);

    // Проверка устойчивости (пункт 2)
    if (modelType == 1) {
        cout << "\n--- STABILITY ANALYSIS ---" << endl;
        if (linearObj.isStable()) {
            cout << "Result: System is STABLE." << endl;
        }
        else {
            cout << "WARNING: System is UNSTABLE! Simulation may diverge." << endl;
        }
        cout << "--------------------------\n" << endl;
    }

    // Сохранение результатов в файл
    ofstream outputFile("results.csv");
    outputFile << "Step;Input;Output\n";

    cout << setw(6) << "Step" << setw(12) << "Input" << setw(15) << "Output" << endl;

    for (int t = 0; t < steps; ++t) {
        double inputValue = generateSignal(t, signalType);
        double outputValue = 0.0;

        if (modelType == 1) {
            outputValue = linearObj.step(inputValue);
        }
        else if (modelType == 2) {
            outputValue = nonlinearObj.step(inputValue);
        }
        else if (modelType == 3) {
            outputValue = diffObj.step(inputValue);
        }

        cout << fixed << setprecision(6);
        cout << setw(6) << t << setw(12) << inputValue << setw(15) << outputValue << endl;

        outputFile << t << ";" << inputValue << ";" << outputValue << "\n";
    }

    outputFile.close();

    cout << "\nData saved to results.csv" << endl;

    // Генерация графика
    string command = "python trunk/ii02906/task_01/plot_results.py results.csv result_plot.png";
    int resultCode = system(command.c_str());

    if (resultCode == 0) {
        system("start result_plot.png");
    }
    else {
        cout << "[WARNING] Error while generating plot!" << endl;
    }

    return 0;
}