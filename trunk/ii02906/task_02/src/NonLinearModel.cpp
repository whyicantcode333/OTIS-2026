#define LAB2_MARKER_02906
#include "NonLinearModel.h"
#include <cmath>

NonLinearModel::NonLinearModel(double a_val, double b_val) : alpha(a_val), beta(b_val) {}

double NonLinearModel::step(double u) {
    double res = alpha * std::tanh(y_t) + beta * std::pow(u, 3);
    y_t = res;
    return res;
}

void NonLinearModel::reset() { y_t = 0; }