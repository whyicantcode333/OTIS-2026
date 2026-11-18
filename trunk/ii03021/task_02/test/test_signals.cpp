// Input signals: step, impulse, harmonic.
#include <gtest/gtest.h>

#include "signals.hpp"

#include <cmath>
#include <string>

using otis::SignalType;

TEST(Signals, StepIsOneForEveryTau) {
    for (int tau : {0, 1, 2, 10, 1000}) {
        EXPECT_DOUBLE_EQ(otis::inputSignal(SignalType::Step, tau), 1.0) << "tau=" << tau;
    }
}

TEST(Signals, ImpulseIsOneOnlyAtZero) {
    EXPECT_DOUBLE_EQ(otis::inputSignal(SignalType::Impulse, 0), 1.0);
    for (int tau : {1, 2, 5, 100}) {
        EXPECT_DOUBLE_EQ(otis::inputSignal(SignalType::Impulse, tau), 0.0) << "tau=" << tau;
    }
    EXPECT_DOUBLE_EQ(otis::inputSignal(SignalType::Impulse, -1), 0.0);
}

TEST(Signals, HarmonicIsSineInRadians) {
    EXPECT_DOUBLE_EQ(otis::inputSignal(SignalType::Harmonic, 0), 0.0);
    EXPECT_NEAR(otis::inputSignal(SignalType::Harmonic, 1), 0.8414709848, 1e-10);
    EXPECT_NEAR(otis::inputSignal(SignalType::Harmonic, 2), 0.9092974268, 1e-10);
    EXPECT_NEAR(otis::inputSignal(SignalType::Harmonic, 3), 0.1411200081, 1e-10);
    EXPECT_NEAR(otis::inputSignal(SignalType::Harmonic, 4), -0.7568024953, 1e-10);
}

TEST(Signals, HarmonicIsBoundedByOne) {
    for (int tau = 0; tau < 1000; ++tau) {
        EXPECT_LE(std::abs(otis::inputSignal(SignalType::Harmonic, tau)), 1.0);
    }
}

TEST(Signals, NamesAreStable) {
    EXPECT_STREQ(otis::signalName(SignalType::Step), "step");
    EXPECT_STREQ(otis::signalName(SignalType::Impulse), "impulse");
    EXPECT_STREQ(otis::signalName(SignalType::Harmonic), "harmonic");
}

TEST(Signals, UnknownSignalTypeFallsBack) {
    const auto bogus = static_cast<SignalType>(99);
    EXPECT_STREQ(otis::signalName(bogus), "unknown");
    EXPECT_DOUBLE_EQ(otis::inputSignal(bogus, 3), 0.0);
}
