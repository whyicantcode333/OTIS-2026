#include "InputSignal.h"
#include <cmath>

double InputSignal::getSignal(SignalType type, int tau) {
    constexpr double signal_amplitude = 1.0;
    switch (type) {
        case SignalType::STEP:
            return signal_amplitude;
        case SignalType::IMPULSE:
            return (tau == 0) ? signal_amplitude : 0.0;
        case SignalType::HARMONIC:
            return signal_amplitude * std::sin(static_cast<double>(tau));
        default:
            return 0.0;
    }
}

std::string InputSignal::getSignalName(SignalType type) {
    switch (type) {
        case SignalType::STEP:
            return "Step";
        case SignalType::IMPULSE:
            return "Impulse";
        case SignalType::HARMONIC:
            return "Harmonic";
        default:
            return "Undefined";
    }
}