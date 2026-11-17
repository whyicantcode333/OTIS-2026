#pragma once
#include <vector>
#include <complex>

// Функция принимает коэффициенты полинома: a_n*z^n + ... + a_1*z + a_0
// Возвращает true, если все корни внутри единичной окружности
bool checkDiscreteStability(const std::vector<double>& coeffs);