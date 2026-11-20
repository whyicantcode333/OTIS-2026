#ifndef INPUT_SIGNAL_H
#define INPUT_SIGNAL_H

#include <string>

enum class SignalType {
    STEP = 1,
    IMPULSE = 2,
    HARMONIC = 3
};

class InputSignal {
private:
    InputSignal() = delete; // Prevent instantiation
public:
    static std::string getSignalName(SignalType type);
    static double getSignal(SignalType type, int tau);
};

#endif