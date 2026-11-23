#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "models.h"
#include "InputSignals.h"

// Интерфейс генератора входных сигналов для подмены в Google Mock
class IInputSignalProvider {
public:
    virtual ~IInputSignalProvider() = default;
    virtual double getSignal(std::size_t step, double time) const = 0;
};

// Адаптер над функцией generateInput из task_01
class StandardInputSignalProvider : public IInputSignalProvider {
private:
    InputSignalType type;
    double amplitude;
    double omega;

public:
    explicit StandardInputSignalProvider(
        InputSignalType type,
        double amplitude = 1.0,
        double omega = 1.0
    );

    double getSignal(std::size_t step, double time) const override;
};

std::vector<double> runSimulation(
    const Model& model,
    const std::vector<double>& u,
    double initialY,
    double dt
);

std::vector<double> runSimulationWithProvider(
    const Model& model,
    const IInputSignalProvider& provider,
    std::size_t steps,
    double initialY,
    double dt
);

bool exportSimulationToHtml(
    const std::string& filename,
    const Model& model,
    const std::vector<double>& u,
    const std::vector<double>& y,
    double dt
);