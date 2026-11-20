#pragma once
#include <cmath>
#include <string>


class Input {
public:
    virtual ~Input() = default;
    virtual double value(int tau) const = 0;
    virtual std::string name() const = 0;
};

// Ступенчатое воздействие
class StepInput : public Input {
public:
    explicit StepInput(double c = 1.0) : c_(c) {}
    double value(int) const override { return c_; }
    std::string name() const override { return "step"; }

private:
    double c_;
};

// Импульсное воздействие
class ImpulseInput : public Input {
public:
    double value(int tau) const override { return tau == 0 ? 1.0 : 0.0; }
    std::string name() const override { return "impulse"; }
};

// Гармоническое воздействие
class HarmonicInput : public Input {
public:
    double value(int tau) const override { return std::sin(static_cast<double>(tau)); }
    std::string name() const override { return "harmonic"; }
};
