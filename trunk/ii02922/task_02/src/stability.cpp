#include "stability.hpp"
#include <cmath>
#include <algorithm>
#include <complex>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Вспомогательная функция для вычисления знаменателя
// Это снижает уровень вложенности в основном цикле
static std::complex<double> computeDenominator(
    const std::vector<std::complex<double>>& roots,
    int n,
    int i
) {
    std::complex<double> den = 1.0;
    for (int j = 0; j < n; ++j) {
        if (i != j) {
            den *= (roots[i] - roots[j]);
        }
    }
    return den;
}

bool checkDiscreteStability(const std::vector<double>& coeffs) {
    int n = static_cast<int>(coeffs.size()) - 1;
    if (n <= 0) return true;

    // Нормировка полинома, чтобы старший коэффициент был равен 1.0
    std::vector<double> p(coeffs.size());
    double lead = coeffs.back();
    if (std::abs(lead) < 1e-14) return false;

    for (size_t i = 0; i < coeffs.size(); ++i) {
        p[i] = coeffs[i] / lead;
    }

    // Инициализация приближенных корней (метод Дюран-Кернера)
    std::vector<std::complex<double>> roots(n);
    double r = 0.4;
    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * M_PI * i / n + 0.3;
        roots[i] = std::complex<double>(r * std::cos(angle), r * std::sin(angle));
    }

    // Итерационный процесс уточнения корней полинома
    const int max_iter = 200;
    for (int iter = 0; iter < max_iter; ++iter) {
        double max_delta = 0.0;
        for (int i = 0; i < n; ++i) {
            std::complex<double> num = p[n];
            for (int j = n - 1; j >= 0; --j) {
                num = num * roots[i] + p[j];
            }

            // ИСПРАВЛЕНИЕ: вынесли вычисление знаменателя в отдельную функцию
            std::complex<double> den = computeDenominator(roots, n, i);

            if (std::abs(den) < 1e-14) continue;

            std::complex<double> delta = num / den;
            roots[i] -= delta;
            max_delta = std::max(max_delta, std::abs(delta));
        }
        if (max_delta < 1e-10) break;
    }

    // Проверка условия: модуль каждого корня должен быть строго меньше 1.0
    const double EPS = 1e-6;
    for (int i = 0; i < n; ++i) {
        if (std::abs(roots[i]) >= (1.0 - EPS)) {
            return false; // Нашелся корень вне единичной окружности -> неустойчиво
        }
    }

    return true;
}