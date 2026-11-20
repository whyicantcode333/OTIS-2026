#pragma once
#include <gmock/gmock.h>

class ISignal {
public:
    virtual ~ISignal() = default;
    virtual double getValue(int tau) const = 0;
};

class MockSignal : public ISignal {
public:
    MOCK_METHOD(double, getValue, (int tau), (const, override));
};