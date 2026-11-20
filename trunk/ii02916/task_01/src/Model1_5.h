#pragma once
#include "Model.h"
#include <queue>
#include <cmath>

class Model1_5 : public Model {
private:
    double x_;
    std::queue<double> delayBuffer_;
    double a1_;
    double a2_;
    double b_;
    int k_;

public:
    Model1_5(double a1, double a2, double b, int k, double initialValue = 0.0)
        : x_(initialValue), a1_(a1), a2_(a2), b_(b), k_(k) {
    }

    double calculateNext(double u, double dt) override {
        delayBuffer_.push(x_);

        int requiredSize = k_;
        if (requiredSize < 1) requiredSize = 1;

        double delayedX = x_;
        if (delayBuffer_.size() > static_cast<size_t>(requiredSize)) {
            delayedX = delayBuffer_.front();
            delayBuffer_.pop();
        }

        double dx = -a1_ * x_ - a2_ * delayedX + b_ * u;
        x_ += dx * dt;
        return x_;
    }

    std::string getName() const override {
        return "Model 1.5 (Линейная)";
    }
};