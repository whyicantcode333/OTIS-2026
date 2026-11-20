#pragma once
#include <string>

class DynamicModel
{
public:
    virtual ~DynamicModel() = default;

    virtual double advance(double input) = 0;
    virtual void reset() = 0;
    virtual std::string describe() const = 0;

    virtual bool verifyStability() const { return true; }
    virtual std::string getWarningMessage() const { return {}; }
};