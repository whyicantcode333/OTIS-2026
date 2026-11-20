#define LAB2_MARKER_02906

#pragma once

class Model
{

public:
    virtual ~Model() = default;
    // метод для расчета следующего шага
    virtual double step(double u) =  0;
    // метод для сброса состояния модели в начальное
    virtual void reset() = 0;
};
