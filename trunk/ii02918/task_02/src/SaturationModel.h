#pragma once
#include "DynamicModel.h"
#include <sstream>
#include <algorithm>

class SaturationModel : public DynamicModel
{
    double coef_a;
    double coef_b;
    double min_val;
    double max_val;
    double cur_y{ 0.0 };

public:
    SaturationModel(double a, double b, double lo, double hi)
        : coef_a(a), coef_b(b), min_val(lo), max_val(hi) {
    }

    double saturate(double v) const
    {
        return std::clamp(v, min_val, max_val);
    }

    double advance(double input) override
    {
        cur_y = coef_a * cur_y + coef_b * saturate(input);
        return cur_y;
    }

    void reset() override { cur_y = 0.0; }

    std::string describe() const override
    {
        std::ostringstream out;
        out << "Model 2.2 (Saturation, Umin=" << min_val << ", Umax=" << max_val << ")";
        return out.str();
    }
};