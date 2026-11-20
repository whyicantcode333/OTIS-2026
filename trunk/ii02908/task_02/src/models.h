#pragma once

#include <string>

class Model { //абстрактный класс для наследования
public:
    virtual double step(double u) = 0; //подсчёт "у" на текущем шаге
    virtual void reset() = 0; //сброс данных перед новым запуском
    virtual ~Model() = default;
};

class LinearModel : public Model { //1.3 y[tau+1] = a1*y[tau] + a2*y[tau-1] + b*u[tau]
private:
    double a1;
    double a2;
    double b;
    double y_prev = 0; // y[tau-1]
    double y_curr = 0; // y[tau]

public:
    LinearModel(double a1_, double a2_, double b_); //конструктор установки параметров уравнения
    double step(double u) override; //вычисление значения на одном шаге
    void reset() override;

    //уравнение 1.3 y[tau+1] = a1*y[tau] + a2*y[tau-1] + b*u[tau] приводим к характеристическому (y[tau] = z^tau) виду:
    //получаем z^2 - a1*z - a2 = 0
    //находим корни и выясняем, что система устойчива, если |z1| < 1 и |z2| < 1
    bool isStable() const; //проверка коэффициентов на устойчивость
};

class NonlinearModel : public Model { //2.10 y[tau+1] = a*tanh(y[tau]) + b*u[tau]^3
private:
    double a;
    double b;
    double y_curr = 0;

public:
    NonlinearModel(double a_, double b_);
    double step(double u) override;
    void reset() override;
};

//при решении методом Эйлера приводим уравнение к виду y = y + b*u*dt
class DifferentialModel : public Model { //3.2 dy/dt = b*u
private:
    double b;
    double dt;
    double y_curr = 0;

public:
    DifferentialModel(double b_, double dt_);
    double step(double u) override;
    void reset() override;
};

double generateSignal(int type, int tau); //генерация входного "u"
void runSimulation(Model* model, int n, int signalType, const std::string& filename); //запуск модели и вывод результатов