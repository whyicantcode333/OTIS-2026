#pragma once
#include "model.hpp"

#include <stdexcept>

namespace otis {

// Block 3 extension: explicit Euler solver of dy/dt = a*y^3 + b*u with a non-zero initial
// state y(0) = y0. The lab 1 model always starts at 0; a non-zero y0 makes it possible to
// compare the numerical result with the analytic solution of dy/dt = a*y^3.
class CubicEulerIvpModel final : public Model {
public:
    CubicEulerIvpModel(double a, double b, double dt, double y0) : a_(a), b_(b), dt_(dt), y0_(y0), y_(y0) {
        if (dt <= 0.0) throw std::invalid_argument("Time step dt must be > 0.");
    }
    double step(double input, int) override {
        y_ += dt_ * (a_ * y_ * y_ * y_ + b_ * input);
        return y_;
    }
    void reset() override { y_ = y0_; }
    [[nodiscard]] const char* name() const override { return "Differential model 3.6 (initial state)"; }

private:
    double a_, b_, dt_, y0_, y_;
};

} // namespace otis
