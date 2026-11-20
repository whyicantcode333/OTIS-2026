#pragma once
// Variant 3: Model 1.3 (linear), Model 2.5 (non-linear), Model 3.7 (ODE, Euler method).
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include "signals.h"

// Abstract base class of all models.
class Model {
public:
    virtual ~Model() = default;

    // Computes y_{tau+1} from the input u_tau and moves the model one step forward.
    virtual double nextStep(double u) = 0;

    // Current output y_tau.
    virtual double current() const = 0;

    virtual std::string name() const = 0;
    virtual std::string tag() const = 0;   // short name for file names, e.g. "model13"
    virtual std::string equation() const = 0;
    virtual std::string params() const = 0;

    // Time step: 1 for discrete models, dt for differential equations.
    virtual double timeStep() const { return 1.0; }

    // Stability check (advanced level). Returns false if the process is not stable.
    // 'info' receives a human-readable description (roots etc.). Default: nothing to check.
    virtual bool isStable(std::string& info) const {
        info.clear();
        return true;
    }
};

// ---------------------------------------------------------------------------
// Model 1.3: second-order linear system
//   y(tau+1) = a1*y(tau) + a2*y(tau-1) + b*u(tau)
// ---------------------------------------------------------------------------
class LinearModel13 : public Model {
public:
    LinearModel13(double a1, double a2, double b, double y0)
        : a1_(a1), a2_(a2), b_(b), yCur_(y0) {}

    double nextStep(double u) override {
        double next = a1_ * yCur_ + a2_ * yPrev_ + b_ * u;
        yPrev_ = yCur_;
        yCur_ = next;
        return yCur_;
    }
    double current() const override { return yCur_; }

    std::string name() const override { return "Model 1.3 (second-order linear)"; }
    std::string tag() const override { return "model13"; }
    std::string equation() const override { return "y(t+1) = a1*y(t) + a2*y(t-1) + b*u(t)"; }
    std::string params() const override {
        return "a1=" + num(a1_) + ", a2=" + num(a2_) + ", b=" + num(b_);
    }

    // Characteristic equation: z^2 - a1*z - a2 = 0. Stable if both roots satisfy |z| < 1.
    bool isStable(std::string& info) const override {
        std::ostringstream os;
        double D = a1_ * a1_ + 4.0 * a2_;
        double maxMod;
        if (D >= 0.0) {
            double s = std::sqrt(D);
            double z1 = (a1_ + s) / 2.0;
            double z2 = (a1_ - s) / 2.0;
            os << "characteristic roots: z1 = " << z1 << ", z2 = " << z2;
            maxMod = std::max(std::fabs(z1), std::fabs(z2));
        } else {
            double re = a1_ / 2.0;
            double im = std::sqrt(-D) / 2.0;
            os << "characteristic roots: z = " << re << " +/- " << im << "i";
            maxMod = std::sqrt(re * re + im * im);
        }
        os << ", max|z| = " << maxMod;
        info = os.str();
        return maxMod < 1.0;
    }

private:
    double a1_;
    double a2_;
    double b_;
    double yCur_;         // y(tau)
    double yPrev_ = 0.0;  // y(tau-1), initial value 0
};

// ---------------------------------------------------------------------------
// Model 2.5: signum friction and exponential growth
//   y(tau+1) = a*y(tau) + b*sign(u(tau))*(1 - exp(-|u(tau)|))
// ---------------------------------------------------------------------------
class NonlinearModel25 : public Model {
public:
    NonlinearModel25(double a, double b, double y0) : a_(a), b_(b), y_(y0) {}

    double nextStep(double u) override {
        y_ = a_ * y_ + b_ * signOf(u) * (1.0 - std::exp(-std::fabs(u)));
        return y_;
    }
    double current() const override { return y_; }

    std::string name() const override { return "Model 2.5 (signum friction, non-linear)"; }
    std::string tag() const override { return "model25"; }
    std::string equation() const override {
        return "y(t+1) = a*y(t) + b*sign(u(t))*(1 - exp(-|u(t)|))";
    }
    std::string params() const override { return "a=" + num(a_) + ", b=" + num(b_); }

private:
    // sign(u): +1, 0 or -1
    static double signOf(double u) {
        if (u > 0.0) return 1.0;
        if (u < 0.0) return -1.0;
        return 0.0;
    }

    double a_;
    double b_;
    double y_;
};

// ---------------------------------------------------------------------------
// Model 3.7: differential equation dy/dt = -exp(a*y) + b*u, Euler method
//   y(k+1) = y(k) + dt * ( -exp(a*y(k)) + b*u(k) )
// ---------------------------------------------------------------------------
class DiffModel37 : public Model {
public:
    DiffModel37(double a, double b, double dt, double y0) : a_(a), b_(b), dt_(dt), y_(y0) {}

    double nextStep(double u) override {
        y_ = y_ + dt_ * (-std::exp(a_ * y_) + b_ * u);
        return y_;
    }
    double current() const override { return y_; }
    double timeStep() const override { return dt_; }

    std::string name() const override { return "Model 3.7 (ODE, Euler method)"; }
    std::string tag() const override { return "model37"; }
    std::string equation() const override { return "dy/dt = -exp(a*y) + b*u"; }
    std::string params() const override {
        return "a=" + num(a_) + ", b=" + num(b_) + ", dt=" + num(dt_);
    }

private:
    double a_;
    double b_;
    double dt_;
    double y_;
};
