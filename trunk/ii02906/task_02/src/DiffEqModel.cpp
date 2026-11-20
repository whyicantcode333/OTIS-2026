#define LAB2_MARKER_02906
#include "DiffEqModel.h"

DiffEqModel::DiffEqModel(double k, double dt) : k_coef(k), step_dt(dt) {}

double DiffEqModel::step(double u) {
    double calc = y_t + step_dt * (-k_coef * y_t * y_t);
    y_t = calc;
    return calc;
}

void DiffEqModel::reset() { y_t = 0; }