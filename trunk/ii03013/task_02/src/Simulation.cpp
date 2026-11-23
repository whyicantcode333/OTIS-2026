#include "Simulation.h"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sstream>

StandardInputSignalProvider::StandardInputSignalProvider(
    InputSignalType type,
    double amplitude,
    double omega
)
    : type(type), amplitude(amplitude), omega(omega) {
}

double StandardInputSignalProvider::getSignal(std::size_t step, double time) const {
    return generateInput(type, step, time, amplitude, omega);
}

std::vector<double> runSimulation(
    const Model& model,
    const std::vector<double>& u,
    double initialY,
    double dt
) {
    std::vector y(u.size(), 0.0);
    if (u.empty()) {
        return y;
    }

    y[0] = initialY;
    for (std::size_t t = 0; t + 1 < u.size(); ++t) {
        const double delayedU = (t >= 2) ? u[t - 2] : 0.0;
        y[t + 1] = model.next(y[t], u[t], delayedU, dt);
    }
    return y;
}

std::vector<double> runSimulationWithProvider(
    const Model& model,
    const IInputSignalProvider& provider,
    std::size_t steps,
    double initialY,
    double dt
) {
    std::vector u(steps + 1, 0.0);
    for (std::size_t t = 0; t <= steps; ++t) {
        u[t] = provider.getSignal(t, static_cast<double>(t) * dt);
    }
    return runSimulation(model, u, initialY, dt);
}

bool exportSimulationToHtml(
    const std::string& filename,
    const Model& model,
    const std::vector<double>& u,
    const std::vector<double>& y,
    double dt
) {
    std::ofstream html(filename);
    if (!html.is_open() || u.size() != y.size() || u.empty()) {
        return false;
    }

    double minVal = std::min(*std::min_element(u.begin(), u.end()), *std::min_element(y.begin(), y.end()));
    double maxVal = std::max(*std::max_element(u.begin(), u.end()), *std::max_element(y.begin(), y.end()));
    if (maxVal - minVal < 1e-9) {
        maxVal += 1.0;
        minVal -= 1.0;
    }

    const int width = 900;
    const int height = 340;
    const int pad = 45;

    auto mapX = [&](std::size_t idx) -> double {
        if (u.size() == 1) return pad;
        return pad + (static_cast<double>(idx) / static_cast<double>(u.size() - 1)) * (width - 2 * pad);
    };
    auto mapY = [&](double val) {
        return (height - pad) - ((val - minVal) / (maxVal - minVal)) * (height - 2 * pad);
    };

    std::ostringstream polyU;
    std::ostringstream polyY;
    polyU << std::fixed << std::setprecision(2);
    polyY << std::fixed << std::setprecision(2);

    const std::size_t stride = std::max<std::size_t>(1, u.size() / 500);
    for (std::size_t i = 0; i < u.size(); i += stride) {
        polyU << mapX(i) << "," << mapY(u[i]) << " ";
        polyY << mapX(i) << "," << mapY(y[i]) << " ";
    }

    const bool stable = model.isStable();
    const std::string badgeClass = stable ? "badge-ok" : "badge-warn";
    const std::string badgeText = stable ? "Устойчива (|z| < 1)" : "Предупреждение: неустойчива (|z| >= 1)";

    html << R"(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<title>Результаты моделирования — )" << model.name() << R"(</title>
<style>
  body { font-family: 'Segoe UI', system-ui, sans-serif; background: #0f172a; color: #e2e8f0; margin: 0; padding: 30px; }
  .container { max-width: 1000px; margin: 0 auto; }
  .card { background:#1e293b; border-radius: 14px; padding: 24px; margin-bottom: 24px; box-shadow: 0 10px 25px rgba(0,0,0,0.3); border: 1px solid #334155; }
  h1 { margin: 0 0 10px 0; font-size: 24px; color: #38bdf8; }
  .badge-ok { display: inline-block; padding: 6px 14px; border-radius: 999px; background: rgba(34,197,94,0.15); color: #4ade80; border: 1px solid #22c55e; font-weight: 600; font-size: 14px; }
  .badge-warn { display: inline-block; padding: 6px 14px; border-radius: 999px; background: rgba(239,68,68,0.15); color: #f87171; border: 1px solid #ef4444; font-weight: 600; font-size: 14px; }
  .legend { display: flex; gap: 20px; margin: 14px 0; font-size: 14px; }
  .dot { width: 12px; height: 12px; border-radius: 50%; display: inline-block; margin-right: 6px; }
  table { width: 100%; border-collapse: collapse; font-size: 14px; }
  th { background:#0f172a; color:#94a3b8; text-align: right; padding: 12px; border-bottom: 2px solid #334155; }
  th:first-child, td:first-child { text-align: center; }
  td { padding: 10px 12px; border-bottom: 1px solid#334155; text-align: right; font-variant-numeric: tabular-nums; }
  tr:hover { background:#263349; }
</style>
</head>
<body>
<div class="container">
  <div class="card">
    <h1>)" << model.name() << R"(</h1>
    <span class=")" << badgeClass << R"(">)" << badgeText << R"(</span>
    <p style="color:#94a3b8; margin-top:12px;">Шаг дискретизации dt = )" << dt << R"( с | Всего шагов: )" << (u.size() - 1) << R"(</p>
    <div class="legend">
      <span><i class="dot" style="background:#38bdf8;"></i>Выход системы y(t)</span>
      <span><i class="dot" style="background:#f59e0b;"></i>Входной сигнал u(t)</span>
    </div>
    <svg viewBox="0 0 )" << width << " " << height << R"(" style="width:100%; background:#0f172a; border-radius:10px; border:1px solid #334155;">
      <line x1=")" << pad << R"(" y1=")" << height - pad << R"(" x2=")" << width - pad << R"(" y2=")" << height - pad << R"(" stroke="#475569" stroke-width="1"/>
      <line x1=")" << pad << R"(" y1=")" << pad << R"(" x2=")" << pad << R"(" y2=")" << height - pad << R"(" stroke="#475569" stroke-width="1"/>
      <polyline fill="none" stroke="#f59e0b" stroke-width="2" stroke-dasharray="5,5" points=")" << polyU.str() << R"("/>
      <polyline fill="none" stroke="#38bdf8" stroke-width="2.5" points=")" << polyY.str() << R"("/>
    </svg>
  </div>
  <div class="card">
    <h2 style="margin-top:0; font-size:18px; color:#f8fafc;">Таблица значений (первые 25 шагов)</h2>
    <table>
      <thead><tr><th>Шаг</th><th>Время t, с</th><th>Вход u(t)</th><th>Выход y(t)</th></tr></thead>
      <tbody>
)";

    html << std::fixed << std::setprecision(6);
    const std::size_t rows = std::min<std::size_t>(u.size(), 25);
    for (std::size_t t = 0; t < rows; ++t) {
        html << "        <tr><td>" << t << "</td><td>" << (static_cast<double>(t) * dt)
             << "</td><td>" << u[t] << "</td><td>" << y[t] << "</td></tr>\n";
    }

    html << R"(      </tbody>
    </table>
  </div>
</div>
</body>
</html>
)";
    return true;
}