#pragma once

#include <string>
#include <vector>

class StabilityAnalyzer {
public:
    static bool isFirstOrderLinearStable(double a1);
    static std::vector<double> getCharacteristicRoots(double a1);
    static std::string getWarningMessage(double a1);
};