#include <iostream>
#include <iomanip>
#include "Model1_9.h"
#include "Model2_6.h"
#include "Model3_9.h"
#include "InputSignal.h"
#include "StabilityChecker.h"
#include "Simulator.h"

int main()
{
    const double a1 = 0.5;
    const double a2 = 0.2;
    const double a3 = 0.1;
    const double b1 = 0.4;

    StabilityChecker::checkAndWarn(a1, a2, a3, std::cout);

    Model1_9 linearModel(a1, a2, a3, b1);
    FunctionSignalSource stepSignal(SignalType::Step);

    const auto trajectory = Simulator::run(linearModel, stepSignal, 10, 1.0, 0.1);

    std::cout << "Модель: " << linearModel.getName() << "\n";
    std::cout << std::fixed << std::setprecision(4);
    for (size_t i = 0; i < trajectory.size(); ++i)
    {
        std::cout << "t=" << i << " -> y=" << trajectory[i] << "\n";
    }

    return 0;
}