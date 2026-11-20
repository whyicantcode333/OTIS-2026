// Model 2.1 implementation - ID: ii02921
#include "Model2_1.h"
#include <cmath>

Model2_1::Model2_1(double a_val, double b_val, double c_val, double d_val, double initial_y)
    : Model(initial_y), a(a_val), b(b_val), c(c_val), d(d_val),
      y_prev(initial_y) {}

double Model2_1::step(double u) {
    const double current_y = getY();
    const double next_y = a * current_y - b * (y_prev * y_prev) + c * u + d * std::sin(u_prev);
    y_prev = current_y;
    u_prev = u;
    setY(next_y);
    return next_y;
}

void Model2_1::reset(double initial_y) {
    Model::reset(initial_y);
    y_prev = initial_y;
    u_prev = 0.0;
}