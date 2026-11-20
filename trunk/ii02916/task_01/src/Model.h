#pragma once
#include <string>

class Model {
public:
    virtual ~Model() = default;

    virtual double calculateNext(double u, double dt) = 0;

    virtual std::string getName() const = 0;
};