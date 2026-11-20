#ifndef MODEL2_1_H
#define MODEL2_1_H

#include "Model.h"

class Model2_1 : public Model {
private:
    double a;
    double b;
    double c;
    double d;
    double y_prev;
    double u_prev = 0.0; // Инициализация прямо в классе для соответствия cpp:S3230

public:
    explicit Model2_1(double a_val, double b_val, double c_val, double d_val, double initial_y = 0.0);
    double step(double u) override;
    void reset(double initial_y = 0.0) override;
};

#endif // MODEL2_1_H