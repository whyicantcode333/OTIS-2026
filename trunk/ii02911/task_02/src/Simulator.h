#pragma once

#include <vector>
#include "Model.h"
#include "InputSignal.h"

class ISignalSource
{
public:
    virtual ~ISignalSource() = default;
    virtual double sample(int step, double amplitude) const = 0;
};

class FunctionSignalSource final : public ISignalSource
{
private:
    SignalType type;

public:
    explicit FunctionSignalSource(SignalType signalType);
    double sample(int step, double amplitude) const override;
};

class Simulator
{
public:
    static std::vector<double> run(
        const Model& model,
        const ISignalSource& signalSource,
        int steps,
        double amplitude,
        double dt,
        double initialValue = 0.0
    );
};