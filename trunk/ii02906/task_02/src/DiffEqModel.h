#define LAB2_MARKER_02906
#pragma once
#include "Model.h"

class DiffEqModel : public Model {
private:
    double k_coef;
    double step_dt;
    double y_t = 0;
public:
    DiffEqModel(double k, double dt);
    double step(double u) override;
    void reset() override;
};