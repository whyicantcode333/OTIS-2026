#pragma once

#include <string>

class Model {
public:
    virtual ~Model() = default;

    virtual double nextStep(double u) = 0;

    virtual std::string name() const = 0;
};

class Model14 : public Model {
public:
    Model14(double a, double b, double y0) : a_(a), b_(b), y_(y0) {}

    double nextStep(double u) override
    {
        y_ = a_ * y_ + b_ * (u - uPrev_);
        uPrev_ = u;
        return y_;
    }

    std::string name() const override { return "Модель 1.4"; }

private:
    double a_;
    double b_;
    double y_;
    double uPrev_ = 0.0;
};

class Model26 : public Model {
public:
    Model26(double a1, double a2, double b, double y0) : a1_(a1), a2_(a2), b_(b), y_(y0) {}

    double nextStep(double u) override
    {
        double next = a1_ * y_ * y_ * y_ - a2_ * yPrev_ + b_ * u * u;
        yPrev_ = y_;
        y_ = next;
        return y_;
    }

    std::string name() const override { return "Модель 2.6"; }

private:
    double a1_;
    double a2_;
    double b_;
    double y_;
    double yPrev_ = 0.0;
};

class Model38 : public Model {
public:
    Model38(double a1, double a2, double b, double dt, double y0)
        : a1_(a1), a2_(a2), b_(b), dt_(dt), y_(y0)
    {
    }

    double nextStep(double u) override
    {
        double derivative = -a1_ * y_ - a2_ * y_ * y_ + b_ * u;
        y_ = y_ + dt_ * derivative;
        return y_;
    }

    std::string name() const override { return "Модель 3.8"; }

private:
    double a1_;
    double a2_;
    double b_;
    double dt_;
    double y_;
};
