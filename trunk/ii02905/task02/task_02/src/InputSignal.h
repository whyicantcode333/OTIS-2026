#pragma once

enum class InputType
{
    Step = 1,
    Impulse = 2,
    Harmonic = 3
};

class InputSignal
{
public:
    static double getSignal(InputType type, int tau);
};