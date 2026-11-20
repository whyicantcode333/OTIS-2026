#define LAB2_MARKER_02906
#pragma once
#include "Model.h"

class NonLinearModel : public Model {
private:
    double alpha;
    double beta;
    double y_t = 0;
public:
    NonLinearModel(double a_val, double b_val);
    double step(double u) override;
    void reset() override;
    bool isWorking() const { return true; } // ”никальный метод
};