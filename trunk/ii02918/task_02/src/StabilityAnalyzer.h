#pragma once

class StabilityAnalyzer {
public:
    static bool isStableStateDelay(double a1, double a2) {
        return (std::abs(a1) + std::abs(a2)) < 1.0;
    }

    static bool isStableEuler(double a1, double a2, double dt, double y0) {
        double multiplier = 1.0 + dt * (-a1 - 2.0 * a2 * y0);
        return std::abs(multiplier) < 1.0;
    }
};