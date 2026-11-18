add_executable(otis_tests
    test_models.cpp
    test_signals.cpp
    test_stability.cpp
    test_simulator.cpp
    test_euler.cpp
)
target_link_libraries(otis_tests PRIVATE otis_core gmock_main)

include(GoogleTest)
gtest_discover_tests(otis_tests DISCOVERY_MODE PRE_TEST)
