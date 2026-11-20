#pragma once
#include <cmath>

// Перечисление типов входных воздействий по заданию
enum class SignalType {
    Step = 1,     // Ступенчатое (u = const)
    Impulse = 2,  // Импульсное (u0 = amplitude, при tau > 0 -> 0)
    Harmonic = 3  // Гармоническое (u = amplitude * sin(tau))
};

// Функция генерации входного сигнала в зависимости от шага tau
inline double getInput(SignalType type, int tau, double amplitude) {
    switch (type) {
        case SignalType::Step:
            return amplitude;
            
        case SignalType::Impulse:
            return (tau == 0) ? amplitude : 0.0;
            
        case SignalType::Harmonic:
            return amplitude * std::sin(static_cast<double>(tau));
            
        default:
            return 0.0;
    }
}