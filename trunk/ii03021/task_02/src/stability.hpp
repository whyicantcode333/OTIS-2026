#pragma once
#include <complex>
#include <iosfwd>
#include <string>
#include <vector>

namespace otis {

// Position of the dominant characteristic root relative to the unit circle of the Z-plane.
enum class Stability {
    Stable,    // all |z| < 1  -> asymptotically stable
    Marginal,  // max |z| == 1 -> boundary case, no convergence to rest
    Unstable   // some |z| > 1 -> divergent process
};

// |z| = 1 is treated as the boundary if the spectral radius differs from 1 by <= tolerance.
constexpr double kUnitCircleTolerance = 1e-9;

struct StabilityReport {
    Stability status = Stability::Stable;
    std::vector<std::complex<double>> roots;  // distinct roots of the characteristic equation
    double spectralRadius = 0.0;              // max |z|
    std::string equation;                     // characteristic equation, human readable
    std::string message;                      // verdict, human readable

    [[nodiscard]] bool isStable() const noexcept { return status == Stability::Stable; }
};

const char* stabilityName(Stability status);

double spectralRadius(const std::vector<std::complex<double>>& roots);
Stability classifySpectralRadius(double radius, double tolerance = kUnitCircleTolerance);

// Model 1.2: y[t+1] = a*y[t] + b*u[t-k].
// Characteristic equation (delay line modelled as k extra states): z^k * (z - a) = 0,
// roots z = a and z = 0 (multiplicity k). Stable <=> |a| < 1; b and k do not affect stability.
StabilityReport analyzeDelayedControl(double a, int delay);

// Model 2.4: y[t+1] = a*y[t]*cos(y[t]) + b*ln(1+|u[t]|).
// Linearization at y = 0 gives z - a = 0, i.e. local stability of the origin <=> |a| < 1.
StabilityReport analyzeLogarithmic(double a);

// Prints the analysis to `out`. If the process is not asymptotically stable, a WARNING line is
// printed. Returns true if a warning was printed.
bool printStabilityReport(const StabilityReport& report, const std::string& modelName,
                          std::ostream& out);

} // namespace otis
