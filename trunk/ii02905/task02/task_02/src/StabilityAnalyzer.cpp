
#include "StabilityAnalyzer.h"

#include <cmath>
#include <vector>
#include <algorithm>

bool StabilityAnalyzer::isStable(double a1, double a2, int k)
{
    if (k < 1 || !std::isfinite(a1) || !std::isfinite(a2))
        return false;

    std::vector<double> coefficients(k + 2, 0.0);

    coefficients[0] = 1.0;
    coefficients[1] = -a1;
    coefficients[k + 1] = -a2;

    const double epsilon = 1e-10;

    while (coefficients.size() > 1)
    {
        int n = static_cast<int>(coefficients.size()) - 1;

        double first = coefficients.front();
        double last = coefficients.back();

        if (std::abs(first) <= epsilon ||
            std::abs(last) >= std::abs(first) - epsilon)
        {
            return false;
        }

        std::vector<double> reduced(n);

        for (int i = 0; i < n; ++i)
        {
            reduced[i] =
                first * coefficients[i] -
                last * coefficients[n - i];
        }

        double scale = std::abs(reduced.front());

        if (scale <= epsilon)
            return false;

        for (double& value : reduced)
            value /= scale;

        coefficients = reduced;
    }

    return true;
}
