#pragma once
#include <cmath>
#include <string>

class SignalGenerator
{
public:
    virtual ~SignalGenerator() = default;
    virtual double value(int step) const = 0;
    virtual std::string getName() const = 0;
};

class StepSignal : public SignalGenerator
{
    double amp;
public:
    explicit StepSignal(double a = 1.0) : amp(a) {}
    double value(int) const override { return amp; }
    std::string getName() const override { return "Step (u = const)"; }
};

class BurstSignal : public SignalGenerator
{
public:
    double value(int step) const override { return (step == 1) ? 1.0 : 0.0; }
    std::string getName() const override { return "Burst (impulse at tau=1)"; }
};

class HarmonicSignal : public SignalGenerator
{
public:
    double value(int step) const override { return std::sin(static_cast<double>(step)); }
    std::string getName() const override { return "Harmonic (u = sin(tau))"; }
};