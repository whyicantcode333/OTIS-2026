#pragma once
#include "Model.h"
#include <cmath>

class Model2_9 : public Model {
private:
    double x_;
    double a_;
    double b_;

public:
    explicit Model2_9(double initialValue = 0.0, double a = 1.0, double b = 1.0)
        : x_(initialValue), a_(a), b_(b) {
    }

    double calculateNext(double u, double dt) override {
        double modulatedU = b_ * std::copysign(std::sqrt(std::abs(u)), u);

        double dx = -a_ * x_ + modulatedU;
        x_ += dx * dt;
        return x_;
    }

    std::string getName() const override {
        return "Square Root Modulated Action";
    }

    void reset(double value = 0.0) {
        x_ = value;
    }
};