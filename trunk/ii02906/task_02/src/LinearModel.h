#define LAB2_MARKER_02906
#pragma once
#include "Model.h"

class LinearModel : public Model {
private:
    double c1;
    double c2;
    double c3;
    double cb;
    double y_t = 0;
    double y_t1 = 0;
    double y_t2 = 0;
public:
    LinearModel(double coef1, double coef2, double coef3, double coef_b);
    double step(double u) override;
    void reset() override;
    bool isStable() const;
};