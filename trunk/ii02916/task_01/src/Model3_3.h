#pragma once
#include "Model.h"

class Model3_3 : public Model {
private:
    double y_;
    double tau_;
    double gain_;

public:
    explicit Model3_3(double initialValue = 0.0, double tau = 1.0, double gain = 1.0)
        : y_(initialValue), tau_(tau), gain_(gain) {
    }

    double calculateNext(double u, double dt) override {
        if (tau_ <= 0.0) {
            y_ = gain_ * u;
            return y_;
        }
        double dy = (gain_ * u - y_) / tau_;
        y_ += dy * dt;
        return y_;
    }

    std::string getName() const override {
        return "Standard First-Order Process";
    }

    void reset(double value = 0.0) {
        y_ = value;
    }
};