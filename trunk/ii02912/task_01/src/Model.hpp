#pragma once
#include <algorithm>
#include <string>
#include <vector>

class Model {
public:
    explicit Model(double y0 = 0.0) : y0_(y0), y_(y0) {}
    virtual ~Model() = default;

    double nextStep(double u) {
        y_ = computeNext(u);
        return y_;
    }

    virtual std::string name() const = 0;
    virtual void reset() { y_ = y0_; }

    double y() const { return y_; }
    double y0() const { return y0_; }

private:
    virtual double computeNext(double u) = 0;

    double y0_;
    double y_;
};

class DistributedControlModel : public Model {
public:
    DistributedControlModel(double a1, double b1, double b2, double y0 = 0.0)
        : Model(y0), a1_(a1), b1_(b1), b2_(b2) {}

    std::string name() const override { return "Model_1_10_DistributedControl"; }

    void reset() override {
        Model::reset();
        hist_.clear();
    }

private:
    double computeNext(double u) override {
        double uDelayed = 0.0;
        if (hist_.size() >= 2) {
            uDelayed = hist_[hist_.size() - 2];
        }
        hist_.push_back(u);
        return a1_ * y() + b1_ * u + b2_ * uDelayed;
    }

    double a1_;
    double b1_;
    double b2_;
    std::vector<double> hist_;   
};

class SaturationModel : public Model {
public:
    SaturationModel(double a, double b, double uMin, double uMax, double y0 = 0.0)
        : Model(y0), a_(a), b_(b), uMin_(uMin), uMax_(uMax) {}

    std::string name() const override { return "Model_2_2_Saturation"; }

private:
    double computeNext(double u) override {
        return a_ * y() + b_ * std::clamp(u, uMin_, uMax_);
    }

    double a_;
    double b_;
    double uMin_;
    double uMax_;
};

class QuadraticDecayModel : public Model {
public:
    QuadraticDecayModel(double a, double h, double y0)
        : Model(y0), a_(a), h_(h) {}

    std::string name() const override { return "Model_3_4_QuadraticDecay"; }


    double exact(double t) const { return y0() / (1.0 + a_ * y0() * t); }
    double h() const { return h_; }

private:
    double computeNext(double /*u*/) override {
        const double cur = y();
        return cur - h_ * a_ * cur * cur;
    }

    double a_;
    double h_;
};
