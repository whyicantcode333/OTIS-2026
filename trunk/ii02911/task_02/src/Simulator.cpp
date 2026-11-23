#include "Simulator.h"
#include <stdexcept>

FunctionSignalSource::FunctionSignalSource(SignalType signalType)
    : type(signalType)
{
}

double FunctionSignalSource::sample(int step, double amplitude) const
{
    return getInput(type, step, amplitude);
}

std::vector<double> Simulator::run(
    const Model& model,
    const ISignalSource& signalSource,
    int steps,
    double amplitude,
    double dt,
    double initialValue)
{
    if (steps < 0)
    {
        throw std::invalid_argument("Количество шагов не может быть отрицательным");
    }

    std::vector<double> history;
    history.reserve(static_cast<size_t>(steps) + 1);

    double y = initialValue;
    double yPrev = initialValue;
    double yPrev2 = initialValue;

    history.push_back(y);

    for (int t = 1; t <= steps; ++t)
    {
        const double u = signalSource.sample(t, amplitude);
        const double yNext = model.calculateNext(y, yPrev, yPrev2, u, dt);

        history.push_back(yNext);

        yPrev2 = yPrev;
        yPrev = y;
        y = yNext;
    }

    return history;
}