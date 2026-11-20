#ifndef MODEL3_7_H
#define MODEL3_7_H

#include "Model.h"

class Model3_7 : public Model {
private:
    double a;
    double b;
    double dt;

public:
    explicit Model3_7(double a_val, double b_val, double dt_val = 0.1, double initial_y = 0.0);
    double step(double u) override;
};

#endif