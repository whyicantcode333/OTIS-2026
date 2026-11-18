#include "stability.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace otis {
namespace {

std::string num(double value) {
    std::ostringstream os;
    os << std::setprecision(10) << value;
    return os.str();
}

std::string formatRoot(const std::complex<double>& z) {
    if (z.imag() == 0.0) return num(z.real());
    return num(z.real()) + (z.imag() < 0.0 ? " - " : " + ") + num(std::abs(z.imag())) + "i";
}

void requireFinite(double a) {
    if (!std::isfinite(a)) throw std::invalid_argument("Coefficient a must be a finite number.");
}

void fillVerdict(StabilityReport& report) {
    report.spectralRadius = spectralRadius(report.roots);
    report.status = classifySpectralRadius(report.spectralRadius);
    const std::string r = num(report.spectralRadius);
    switch (report.status) {
    case Stability::Stable:
        report.message = "All roots lie strictly inside the unit circle (max |z| = " + r +
                         " < 1): the process is asymptotically stable.";
        break;
    case Stability::Marginal:
        report.message = "A root lies on the unit circle (max |z| = " + r +
                         "): the process is only marginally stable and does not converge to rest.";
        break;
    case Stability::Unstable:
        report.message = "A root lies outside the unit circle (max |z| = " + r +
                         " > 1): the process is unstable and its output will diverge.";
        break;
    }
}

} // namespace

const char* stabilityName(Stability status) {
    switch (status) {
    case Stability::Stable: return "stable";
    case Stability::Marginal: return "marginal";
    case Stability::Unstable: return "unstable";
    }
    return "unknown";
}

double spectralRadius(const std::vector<std::complex<double>>& roots) {
    double radius = 0.0;
    for (const auto& z : roots) radius = std::max(radius, std::abs(z));
    return radius;
}

Stability classifySpectralRadius(double radius, double tolerance) {
    if (radius < 1.0 - tolerance) return Stability::Stable;
    if (radius <= 1.0 + tolerance) return Stability::Marginal;
    return Stability::Unstable;  // also catches NaN
}

StabilityReport analyzeDelayedControl(double a, int delay) {
    if (delay < 1) throw std::invalid_argument("Delay k must be >= 1.");
    requireFinite(a);
    StabilityReport report;
    report.roots = {{a, 0.0}, {0.0, 0.0}};
    report.equation = (delay == 1 ? std::string("z") : "z^" + std::to_string(delay)) +
                      " * (z - " + num(a) + ") = 0";
    fillVerdict(report);
    return report;
}

StabilityReport analyzeLogarithmic(double a) {
    requireFinite(a);
    StabilityReport report;
    report.roots = {{a, 0.0}};
    report.equation = "z - " + num(a) + " = 0 (linearization at y = 0)";
    fillVerdict(report);
    return report;
}

bool printStabilityReport(const StabilityReport& report, const std::string& modelName,
                          std::ostream& out) {
    out << "\nStability analysis: " << modelName << '\n'
        << "  Characteristic equation: " << report.equation << '\n'
        << "  Roots on the Z-plane:";
    for (std::size_t i = 0; i < report.roots.size(); ++i) {
        out << (i == 0 ? " " : ", ") << "z" << (i + 1) << " = " << formatRoot(report.roots[i]);
    }
    out << "\n  Spectral radius max|z| = " << num(report.spectralRadius) << '\n';
    if (report.isStable()) {
        out << "  [OK] " << report.message << '\n';
        return false;
    }
    out << "  [WARNING] " << report.message << '\n';
    return true;
}

} // namespace otis
