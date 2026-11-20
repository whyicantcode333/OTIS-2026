cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) 
project(ii02918_task_02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ii02918_task_02
    src/main.cpp
    src/DynamicModel.h
    src/StateDelayModel.h
    src/SaturationModel.h
    src/QuadraticDecayODE.h
    src/SignalGenerator.h
    src/ModelCreator.h
)

add_executable(run_tests
    test/tests.cpp
    src/DynamicModel.h
    src/StateDelayModel.h
    src/SaturationModel.h
    src/QuadraticDecayODE.h
    src/SignalGenerator.h
    src/StabilityAnalyzer.h
)

target_compile_definitions(run_tests PRIVATE GTEST_COLOR=yes)

target_include_directories(run_tests PRIVATE src)

target_link_options(run_tests PRIVATE --coverage)

target_link_libraries(run_tests PRIVATE GTest::gtest_main GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(run_tests)