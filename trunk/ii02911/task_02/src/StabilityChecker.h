#pragma once

#include <ostream>

class StabilityChecker
{
public:
    // Проверяет устойчивость линейной дискретной системы 3-го порядка:
    // y[k] = a1*y[k-1] + a2*y[k-2] + a3*y[k-3] + b1*u
    // Характеристический полином: P(z) = z^3 - a1*z^2 - a2*z - a3
    static bool isModel1_9Stable(double a1, double a2, double a3);

    // Проверяет устойчивость и при неустойчивости выводит предупреждение в поток
    static bool checkAndWarn(double a1, double a2, double a3, std::ostream& out);
};