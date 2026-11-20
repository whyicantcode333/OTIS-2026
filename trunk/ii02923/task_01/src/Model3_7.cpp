#include "Model3_7.h"
#include <cmath>

Model3_7::Model3_7(double a_val, double b_val, double dt_val, double initial_y)
    : Model(initial_y), a(a_val), b(b_val), dt(dt_val) {}

double Model3_7::step(double u) {
    const double current_y = getY();
    const double dy = -std::exp(a) * current_y + b * u;
    const double next_y = current_y + dy * dt;
    setY(next_y);
    return next_y;
}