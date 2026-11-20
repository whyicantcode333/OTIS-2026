#include "Model1_6.h"

Model1_6::Model1_6(double a1_val, double a2_val, double a3_val, double b_val, double initial_y)
    : Model(initial_y), a1(a1_val), a2(a2_val), a3(a3_val), b(b_val),
      y_prev1(initial_y), y_prev2(initial_y) {}

double Model1_6::step(double u) {
    const double current_y = getY();
    const double next_y = a1 * current_y + a2 * y_prev1 + a3 * y_prev2 + b * u;
    y_prev2 = y_prev1;
    y_prev1 = current_y;
    setY(next_y);
    return next_y;
}

void Model1_6::reset(double initial_y) {
    Model::reset(initial_y);
    y_prev1 = initial_y;
    y_prev2 = initial_y;
}