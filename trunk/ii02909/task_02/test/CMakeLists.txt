add_executable(run_tests
    test_main.cpp
    test_model1_4.cpp
    test_model2_9.cpp
    test_model3_1.cpp
    test_stability.cpp
    test_signals.cpp
)

target_link_libraries(run_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
)

target_include_directories(run_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)