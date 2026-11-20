#define LAB2_MARKER_02906
#include "LinearModel.h"
#include <cmath>

LinearModel::LinearModel(double coef1, double coef2, double coef3, double coef_b)
    : c1(coef1), c2(coef2), c3(coef3), cb(coef_b) {
}

double LinearModel::step(double u) {
    double result = c1 * y_t + c2 * y_t1 + c3 * y_t2 + cb * u;
    y_t2 = y_t1;
    y_t1 = y_t;
    y_t = result;
    return result;
}

void LinearModel::reset() {
    y_t = 0; y_t1 = 0; y_t2 = 0;
}

bool LinearModel::isStable() const {
    if (std::abs(c1) >= 1) return false;
    if (1 + c1 + c2 + c3 <= 0) return false;
    if (1 - c1 + c2 - c3 <= 0) return false;
    if (std::abs(c3 * c1 - 1) <= std::abs(c3 * c1 - c2)) return false;
    return true;
}