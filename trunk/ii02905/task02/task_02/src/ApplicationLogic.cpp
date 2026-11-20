#include "ApplicationLogic.h"

#include "Simulation.h"

#include <iostream>
#include <iomanip>

using namespace std;

void ApplicationLogic::run()
{
    int modelChoice;
    int inputChoice;
    int steps;

    cout << "====================================" << endl;
    cout << "       Simulation Application       " << endl;
    cout << "====================================" << endl;

    cout << "\nChoose model:" << endl;
    cout << "1 - Model 1.5 (Linear with delay)" << endl;
    cout << "2 - Model 2.7 (Relay model)" << endl;
    cout << "3 - Model 3.9 (Euler method)" << endl;
    cout << "Your choice: ";
    cin >> modelChoice;

    if (modelChoice < 1 || modelChoice > 3)
    {
        cout << "Error: invalid model choice!" << endl;
        return;
    }

    cout << "\nChoose input signal:" << endl;
    cout << "1 - Step" << endl;
    cout << "2 - Impulse" << endl;
    cout << "3 - Harmonic" << endl;
    cout << "Your choice: ";
    cin >> inputChoice;

    if (inputChoice < 1 || inputChoice > 3)
    {
        cout << "Error: invalid input signal!" << endl;
        return;
    }

    cout << "\nEnter number of simulation steps: ";
    cin >> steps;

    if (steps <= 0)
    {
        cout << "Error: number of steps must be positive!" << endl;
        return;
    }

    InputType inputType =
        static_cast<InputType>(inputChoice);

    SimulationResult result =
        Simulation::run(modelChoice, inputType, steps);

    if (result.output.empty())
    {
        cout << "Simulation failed!" << endl;
        return;
    }

    cout << "\nSimulation results:" << endl;

    cout << left
         << setw(10) << "Time"
         << setw(15) << "Input"
         << setw(15) << "Output"
         << endl;

    cout << "----------------------------------------" << endl;

    for (size_t i = 0; i < result.output.size(); i++)
    {
        cout << left
             << setw(10) << result.time[i]
             << setw(15) << fixed << setprecision(4) << result.input[i]
             << setw(15) << result.output[i]
             << endl;
    }
}