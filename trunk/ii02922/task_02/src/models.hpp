#pragma once
#include <iostream>
#include <cmath>
#include <string>

// ============================================================
// ¡¿«Œ¬€… ¿¡—“–¿ “Õ€…  À¿—— MODEL
// ============================================================
class Model
{
public:
    virtual double calculateNext(double y, double u, double dt) = 0;
    virtual std::string getName() const = 0;
    virtual void reset() = 0;
    virtual ~Model() = default;
};

// ============================================================
// ÃŒƒ≈À‹ 1.10
// y(t+1) = a1*y(t) + b1*u(t) + b2*u(t-2)
// ============================================================
class Model1_10 : public Model
{
private:
    double m_a1;
    double m_b1;
    double m_b2;

    double m_u_t_minus_1 = 0.0;
    double m_u_t_minus_2 = 0.0;

public:
    Model1_10(double a1, double b1, double b2)
        : m_a1(a1), m_b1(b1), m_b2(b2) {
    }

    double calculateNext(double y, double u, double dt) override
    {
        (void)dt;
        double nextY = m_a1 * y + m_b1 * u + m_b2 * m_u_t_minus_2;

        m_u_t_minus_2 = m_u_t_minus_1;
        m_u_t_minus_1 = u;

        return nextY;
    }

    std::string getName() const override
    {
        return "Model 1.10 - Distributed Control Window Model";
    }

    void reset() override
    {
        m_u_t_minus_1 = 0.0;
        m_u_t_minus_2 = 0.0;
    }
};

// ============================================================
// ÃŒƒ≈À‹ 2.4
// y(t+1) = a*y(t)*cos(y(t)) + b*ln(1+|u(t)|)
// ============================================================
class Model2_4 : public Model
{
private:
    double m_a;
    double m_b;

public:
    Model2_4(double a, double b) : m_a(a), m_b(b) {}

    double calculateNext(double y, double u, double dt) override
    {
        (void)dt;
        return m_a * y * cos(y) + m_b * log(1.0 + fabs(u));
    }

    std::string getName() const override
    {
        return "Model 2.4 - Logarithmic Multi-Variable Interaction";
    }

    void reset() override {}
};

// ============================================================
// ÃŒƒ≈À‹ 3.8
// dy/dt = -a1*y - a2*y^2 + b*u (ÏÂÚÓ‰ ›ÈÎÂ‡)
// ============================================================
class Model3_8 : public Model
{
private:
    double m_a1;
    double m_a2;
    double m_b;

public:
    Model3_8(double a1, double a2, double b) : m_a1(a1), m_a2(a2), m_b(b) {}

    double calculateNext(double y, double u, double dt) override
    {
        double dydt = -m_a1 * y - m_a2 * y * y + m_b * u;
        return y + dt * dydt;
    }

    std::string getName() const override
    {
        return "Model 3.8 - Combined Linear-Quadratic Decay";
    }

    void reset() override {}
};

// ============================================================
// ‘”Õ ÷»ﬂ ‘Œ–Ã»–Œ¬¿Õ»ﬂ ¬’ŒƒÕŒ√Œ —»√Õ¿À¿
// ============================================================
inline double getInput(int type, int t)
{
    if (type == 1) return 1.0;
    if (type == 2) {
        return (t == 0) ? 1.0 : 0.0;
    }
    if (type == 3) return sin(t);
    return 0.0;
}
