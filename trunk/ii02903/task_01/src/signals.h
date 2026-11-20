#pragma once
// Input signals u_tau used to test the models.
#include <cmath>
#include <sstream>
#include <string>

inline std::string num(double v) {
    std::ostringstream os;
    os << v;
    return os.str();
}

// Abstract input signal.
// tau - step number (0, 1, 2, ...), t - physical time (t = tau * dt, for discrete models dt = 1).
class InputSignal {
public:
    virtual ~InputSignal() = default;
    virtual double value(int tau, double t) const = 0;
    virtual std::string name() const = 0;
    virtual std::string tag() const = 0;   // short name for file names
};

// Step: u = const
class StepInput : public InputSignal {
public:
    explicit StepInput(double c) : c_(c) {}
    double value(int, double) const override { return c_; }
    std::string name() const override { return "step (u = " + num(c_) + ")"; }
    std::string tag() const override { return "step"; }
private:
    double c_;
};

// Impulse: u_0 = 1, u_tau = 0 for tau > 0
class ImpulseInput : public InputSignal {
public:
    double value(int tau, double) const override { return tau == 0 ? 1.0 : 0.0; }
    std::string name() const override { return "impulse (u0 = 1, u>0 = 0)"; }
    std::string tag() const override { return "impulse"; }
};

// Harmonic: u = sin(tau)  (for ODE: sin(t), t = tau * dt; for discrete models t = tau)
class HarmonicInput : public InputSignal {
public:
    double value(int, double t) const override { return std::sin(t); }
    std::string name() const override { return "harmonic (u = sin(tau))"; }
    std::string tag() const override { return "harmonic"; }
};
