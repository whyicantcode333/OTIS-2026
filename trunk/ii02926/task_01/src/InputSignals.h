#pragma once

#include <cmath>
#include <string>

class InputSignal {
public:
    virtual ~InputSignal() = default;
    virtual double value(int tau) const = 0;
    virtual std::string name() const = 0;
};

// Ступенчатое: u_tau = const
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
        return "Step (u = const = " + std::to_string(amplitude) + ")";
    }
};

// Импульсное: u_1 = 1, u_{tau>1} = 0
class ImpulseInput : public InputSignal {
public:
    double value(int tau) const override {
        return (tau == 1) ? 1.0 : 0.0;
    }

    std::string name() const override {
        return "Impulse (u_1 = 1, u_tau>1 = 0)";
    }
};

// Гармоническое: u_tau = sin(tau)
class HarmonicInput : public InputSignal {
public:
    double value(int tau) const override {
        return std::sin(static_cast<double>(tau));
    }

    std::string name() const override {
        return "Harmonic (u_tau = sin(tau))";
    }
};