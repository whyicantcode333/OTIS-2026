#include "models.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int main() {
    int n;
    short option = 0;

    cout << "choose your model:\n\n1. linear\n2. nonlinear\n3. differential\n";
    while (option < 1 || option > 3) {
        cout << "\nyour choice: "; cin >> option;
    }

    cout << "\nEnter number of iterations: "; cin >> n;
    if (n <= 0) {
        cout << "Wrong value"; exit(1);
    }

    if (option == 1) {
        double a;
        double b;
        double c;
        cout << "Enter linear coefficients:\na1: "; cin >> a;
        cout << "a2: "; cin >> b;
        cout << "b: "; cin >> c;

        LinearModel lin(a, b, c);
        if (!lin.isStable()) {
            cout << "\nDivirgent process\n";
        }
        runSimulation(&lin, n, 0, "linear_step.csv");
        runSimulation(&lin, n, 1, "linear_impulse.csv");
        runSimulation(&lin, n, 2, "linear_harmonic.csv");

        cout << "All linear processes have been completed\n\n";

        //я знаю, что жёсткий путь - это нехорошо, но у меня иначе не запускается gnuplot :(
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" linear_step.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" linear_impulse.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" linear_harmonic.gp)");

        //открытие графиков
        system("plot_linear_step.png");
        system("plot_linear_impulse.png");
        system("plot_linear_harmonic.png");
    }

    if (option == 2) {
        double a;
        double b;
        cout << "Enter nonlinear coefficients:\na: "; cin >> a;
        cout << "b: "; cin >> b;

        NonlinearModel nonlin(a, b);
        runSimulation(&nonlin, n, 0, "nonlinear_step.csv");
        runSimulation(&nonlin, n, 1, "nonlinear_impulse.csv");
        runSimulation(&nonlin, n, 2, "nonlinear_harmonic.csv");

        cout << "All nonlinear processes have been completed\n\n";

        //жёсткий путь это всё ещё плохо...
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" nonlinear_step.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" nonlinear_impulse.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" nonlinear_harmonic.gp)");

        system("plot_nonlinear_step.png");
        system("plot_nonlinear_impulse.png");
        system("plot_nonlinear_harmonic.png");
    }

    if (option == 3) {
        double b;
        double t;
        cout << "Enter differential coefficients:\nb: "; cin >> b;
        cout << "t changes: "; cin >> t;

        DifferentialModel dif(b, t);
        runSimulation(&dif, n, 0, "dif_step.csv");
        runSimulation(&dif, n, 1, "dif_impulse.csv");
        runSimulation(&dif, n, 2, "dif_harmonic.csv");

        //...но это и правда вынужденная мера
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" dif_step.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" dif_impulse.gp)");
        system(R"("C:\Program Files\gnuplot\bin\gnuplot.exe" dif_harmonic.gp)");

        //интересный факт: автоматическая проверка кода вынуждает меня использовать
        //так называемые "сырые строки" формата R"(...)", несмотря на то, что с
        //ними программа не запускается вовсе

        //системные вызовы также не идеальны, эти строки запустятся только на windows
        system("plot_dif_step.png");
        system("plot_dif_impulse.png");
        system("plot_dif_harmonic.png");
    }
    return 0;
}