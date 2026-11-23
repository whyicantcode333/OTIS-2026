#include "StabilityAnalyzer.h"
#include <cmath>
#include <sstream>

bool StabilityAnalyzer::isFirstOrderLinearStable(double a1) {
    if (!std::isfinite(a1)) {
        return false;
    }
    return std::abs(a1) < 1.0;
}

std::vector<double> StabilityAnalyzer::getCharacteristicRoots(double a1) {
    // Корни уравнения z^2 * (z - a1) = 0: z1 = 0, z2 = 0, z3 = a1
    return { 0.0, 0.0, a1 };
}

std::string StabilityAnalyzer::getWarningMessage(double a1) {
    if (isFirstOrderLinearStable(a1)) {
        return "";
    }

    std::ostringstream oss;
    oss << "[WARNING] System is NOT asymptotically stable! "
        << "Pole |z| = |a1| = " << std::abs(a1) << " >= 1.0 "
        << "(divergent or boundary behavior expected).";
    return oss.str();
}