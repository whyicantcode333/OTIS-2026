#include "models.h"
#include <cmath>
#include <fstream>
#include <iomanip>

// --- LinearModel ---

LinearModel::LinearModel(double a1_, double a2_, double b_) : a1(a1_), a2(a2_), b(b_) {
    //конструктор установки параметров уравнения 
}

double LinearModel::step(double u) { //вычисление значения на одном шаге
    double y_next = a1 * y_curr + a2 * y_prev + b * u;
    y_prev = y_curr;
    y_curr = y_next;
    return y_curr;
}

void LinearModel::reset() {
    y_prev = 0;
    y_curr = 0;
}

//уравнение 1.3 y[tau+1] = a1*y[tau] + a2*y[tau-1] + b*u[tau] приводим к характеристическому (y[tau] = z^tau) виду:
//получаем z^2 - a1*z - a2 = 0
//находим корни и выясняем, что система устойчива, если |z1| < 1 и |z2| < 1
bool LinearModel::isStable() const { //проверка коэффициентов на устойчивость
    double disc = a1 * a1 + 4 * a2; //дискриминант
    if (disc >= 0) { //елси дискриминант больше нуля, то корни вещественные
        double z1 = (a1 + std::sqrt(disc)) / 2;
        double z2 = (a1 - std::sqrt(disc)) / 2;
        return std::fabs(z1) < 1 && std::fabs(z2) < 1;
    }
    else { //если дискриминант меньше нуля, то корни комплексные
        return std::sqrt(-a2) < 1;
    }
}

// --- NonlinearModel ---

NonlinearModel::NonlinearModel(double a_, double b_) : a(a_), b(b_) {

}

double NonlinearModel::step(double u) {
    y_curr = a * std::tanh(y_curr) + b * u * u * u;
    return y_curr;
}

void NonlinearModel::reset() {
    y_curr = 0;
}

// --- DifferentialModel ---

DifferentialModel::DifferentialModel(double b_, double dt_) : b(b_), dt(dt_) {

}

double DifferentialModel::step(double u) {
    y_curr = y_curr + b * u * dt;
    return y_curr;
}

void DifferentialModel::reset() {
    y_curr = 0;
}

// --- Свободные функции ---

double generateSignal(int type, int tau) { //генерация входного "u"
    switch (type) {
    case 0: return tau; //ступенчатое
    case 1: return (tau == 0) ? 1.0 : 0.0; //импульсное
    case 2: return std::sin(tau); //гармоническое
    default: return 0.0;
    }
}

void runSimulation(Model* model, int n, int signalType, const std::string& filename) { //запуск модели и вывод результатов
    std::ofstream fout(filename);
    fout << std::fixed << std::setprecision(6); //настройка формата вывода

    model->reset(); //используем указатель абстрактного родительского класса, т.к. он может хранить ссылку на любой класс-наследник

    for (int tau = 0; tau < n; tau++) {
        double u = generateSignal(signalType, tau);
        double y = model->step(u);

        fout << tau << "," << u << "," << y << "\n";
    }

    fout.close();
}