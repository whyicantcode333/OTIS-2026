#pragma once
// ============================================================
// Заголовочный файл для генераторов входных сигналов.
// Используется в ЛР №2 для тестирования моделей.
// ============================================================

#include <cmath>
#include <string>

// Базовый абстрактный класс для всех входных воздействий
class InputSignal {
public:
    virtual ~InputSignal() = default;
    // Получить значение сигнала на шаге tau (начинается с 1)
    virtual double value(int tau) const = 0;
    // Название сигнала (для вывода в консоль)
    virtual std::string name() const = 0;
};

// Ступенчатое воздействие (постоянное значение)
class StepInput : public InputSignal {
private:
    static constexpr double DEFAULT_AMPLITUDE = 1.0;
    double amplitude;

public:
    explicit StepInput(double amp = DEFAULT_AMPLITUDE) : amplitude(amp) {}

    double value(int /*tau*/) const override {
        return amplitude;
    }

    std::string name() const override {
        return "Constant Step Signal (A = " + std::to_string(amplitude) + ")";
    }
};

// Импульсное воздействие (одиночный скачок на первом шаге)
class ImpulseInput : public InputSignal {
public:
    double value(int tau) const override {
        return (tau == 1) ? 1.0 : 0.0;
    }
    std::string name() const override {
        return "Single Impulse (tau=1)";
    }
};

// Гармоническое воздействие (синусоида)
class HarmonicInput : public InputSignal {
public:
    double value(int tau) const override {
        return std::sin(static_cast<double>(tau));
    }
    std::string name() const override {
        return "Harmonic Sine Wave";
    }
};