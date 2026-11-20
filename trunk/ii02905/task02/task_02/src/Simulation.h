
#pragma once

#include <vector>
#include "InputSignal.h"

struct SimulationResult
{
    std::vector<double> time;
    std::vector<double> input;
    std::vector<double> output;
};

class Simulation
{
public:
    static SimulationResult run(
        int modelChoice,
        InputType inputType,
        int steps
    );
};
