
#include "Simulation.h"

#include "Model.h"
#include "Model15.h"
#include "Model27.h"
#include "Model39.h"

#include <memory>

SimulationResult Simulation::run(
    int modelChoice,
    InputType inputType,
    int steps)
{
    SimulationResult result;

    if (steps <= 0)
        return result;

    std::unique_ptr<Model> model;

    switch (modelChoice)
    {
    case 1:
        model = std::make_unique<Model15>(
            0.0, 0.6, 0.2, 0.5, 2, steps
        );
        break;

    case 2:
        model = std::make_unique<Model27>(
            0.0, 0.8, 0.7, 0.2
        );
        break;

    case 3:
        model = std::make_unique<Model39>(
            0.0, 1.0, 0.1
        );
        break;

    default:
        return result;
    }

    for (int tau = 0; tau < steps; tau++)
    {
        double u = InputSignal::getSignal(inputType, tau);

        double y = model->step(u, tau);

        result.time.push_back(tau);
        result.input.push_back(u);
        result.output.push_back(y);
    }

    return result;
}
