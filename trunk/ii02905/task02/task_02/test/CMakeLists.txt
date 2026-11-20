add_executable(test_models
    test_models.cpp
    ../src/Model15.cpp
    ../src/Model27.cpp
    ../src/Model39.cpp
)

target_include_directories(test_models PRIVATE ../src)

add_test(NAME ModelTests COMMAND test_models)

add_executable(test_input
    test_main_input.cpp
    ../src/InputSignal.cpp
)

target_include_directories(test_input PRIVATE ../src)

add_test(NAME InputSignalTests COMMAND test_input)

add_executable(test_stability
    test_stability.cpp
    ../src/StabilityAnalyzer.cpp
)

target_include_directories(test_stability PRIVATE ../src)

add_test(NAME StabilityTests COMMAND test_stability)

add_executable(test_application_logic
    test_application_logic.cpp
    ../src/ApplicationLogic.cpp
    ../src/Simulation.cpp
    ../src/InputSignal.cpp
    ../src/Model15.cpp
    ../src/Model27.cpp
    ../src/Model39.cpp
)

target_include_directories(test_application_logic PRIVATE ../src)

add_test(NAME ApplicationLogicTests COMMAND test_application_logic)