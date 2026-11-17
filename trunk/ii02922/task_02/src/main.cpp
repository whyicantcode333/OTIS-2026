#include <iostream>
#include <fstream>
#include <string>
#include <clocale>
#include <cstdlib>
#include <memory>
#include <vector>
#include "stability.hpp"
#include "models.hpp"

using namespace std;

// ============================================================
// ФУНКЦИЯ МОДЕЛИРОВАНИЯ
// ============================================================

void simulate(Model& model, int inputType, int n, double y0, double dt, const string& fileName)
{
    cout << "\n==============================================\n";
    cout << model.getName() << "\n";
    cout << "==============================================\n\n";

    cout << "t\tu\t\ty\n";
    cout << "----------------------------------------------\n";

    model.reset();
    double y = y0;

    ofstream file(fileName);
    if (!file.is_open()) {
        cout << "Ошибка открытия файла!\n";
        return;
    }

    file << "t,u,y\n";

    for (int t = 0; t < n; t++) {
        double u = getInput(inputType, t);
        double nextY = model.calculateNext(y, u, dt);

        cout << t << "\t" << u << "\t\t" << nextY << "\n";
        file << t << "," << u << "," << nextY << "\n";

        y = nextY;
    }

    file.close();
    cout << "\nРезультаты сохранены в файл: " << fileName << "\n";
}

// ============================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================

int main()
{
    setlocale(LC_ALL, "");

    cout << "==============================================\n";
    cout << "        ПРОГРАММА МОДЕЛИРОВАНИЯ\n";
    cout << "==============================================\n";

    int n;
    cout << "\nВведите количество шагов моделирования n: ";
    cin >> n;
    if (n <= 0) {
        cout << "Количество шагов должно быть больше 0.\n";
        return 1;
    }

    int modelChoice;
    cout << "\nВыберите модель:\n1 - Model 1.10\n2 - Model 2.4\n3 - Model 3.8\nВаш выбор: ";
    cin >> modelChoice;

    int inputChoice;
    cout << "\nВыберите входное воздействие:\n1 - Ступенчатое\n2 - Импульсное\n3 - Гармоническое\nВаш выбор: ";
    cin >> inputChoice;
    if (inputChoice < 1 || inputChoice > 3) {
        cout << "Неверный тип воздействия.\n";
        return 1;
    }

    double y0;
    cout << "\nВведите начальное значение y: ";
    cin >> y0;

    double dt;
    cout << "Введите шаг времени dt: ";
    cin >> dt;

    if (dt <= 0) {
        cout << "dt должен быть больше 0.\n";
        return 1;
    }

    unique_ptr<Model> model = nullptr;
    string fileName;

    if (modelChoice == 1) {
        double a1, b1, b2;
        cout << "\nВведите коэффициенты Model 1.10:\na1 = "; cin >> a1;
        cout << "b1 = "; cin >> b1;
        cout << "b2 = "; cin >> b2;

        model = make_unique<Model1_10>(a1, b1, b2);
        fileName = "model1_10.csv";

        // Проверка устойчивости для Модели 1.10 (полином z - a1 = 0)
        vector<double> poly = { -a1, 1.0 };
        if (checkDiscreteStability(poly)) {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 1.10 УСТОЙЧИВА.\n";
        }
        else {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 1.10 НЕУСТОЙЧИВА!\n";
        }
    }
    else if (modelChoice == 2) {
        double a, b;
        cout << "\nВведите коэффициенты Model 2.4:\na = "; cin >> a;
        cout << "b = "; cin >> b;

        model = make_unique<Model2_4>(a, b);
        fileName = "model2_4.csv";

        // Проверка устойчивости для Модели 2.4 (линеаризованный полином z - a = 0)
        vector<double> poly = { -a, 1.0 };
        if (checkDiscreteStability(poly)) {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 2.4 УСТОЙЧИВА.\n";
        }
        else {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 2.4 НЕУСТОЙЧИВА!\n";
        }
    }
    else if (modelChoice == 3) {
        double a1, a2, b;
        cout << "\nВведите коэффициенты Model 3.8:\na1 = "; cin >> a1;
        cout << "a2 = "; cin >> a2;
        cout << "b = "; cin >> b;

        model = make_unique<Model3_8>(a1, a2, b);
        fileName = "model3_8.csv";

        // Проверка устойчивости для Модели 3.8 (линеаризованный полином z - (1 - dt * a1) = 0)
        double pole = 1.0 - dt * a1;
        vector<double> poly = { -pole, 1.0 };
        if (checkDiscreteStability(poly)) {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 3.8 УСТОЙЧИВА.\n";
        }
        else {
            cout << "[АНАЛИЗ УСТОЙЧИВОСТИ]: Модель 3.8 НЕУСТОЙЧИВА!\n";
        }
    }
    else {
        cout << "Неверный номер модели.\n";
        return 1;
    }

    if (model) {
        simulate(*model, inputChoice, n, y0, dt, fileName);
        system("python plot.py");
    }

    cout << "\n==============================================\n";
    cout << "          Моделирование завершено.\n";
    cout << "==============================================\n";

    return 0;
}