cmake_minimum_required(VERSION 3.5)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(task_02_tests CXX)
set(ENV{CMAKE_POLICY_VERSION_MINIMUM} 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Принудительная генерация PDB-файлов для всех конфигураций (включая Release)
if (MSVC)
    add_compile_options(/Zi)
    add_link_options(/DEBUG)
endif()

# ---- Visual Studio 2022 (MSVC) ----
if (NOT MSVC)
    message(FATAL_ERROR "This project is configured for MSVC (Visual Studio 2022).")
endif()

# ---- GoogleTest ----
include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

    FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
    CMAKE_CACHE_ARGS "-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5"
)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

# ---- Исходники проекта ----
set(SRC_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")

add_library(models_lib STATIC
    "${SRC_DIR}/models.cpp"
)
target_include_directories(models_lib PUBLIC "${SRC_DIR}")

# ---- Тестовый исполняемый файл ----
add_executable(tests
    "${TEST_DIR}/test_models_parametrized.cpp"
    "${TEST_DIR}/test_run_mockmodel.cpp"
    "${TEST_DIR}/test_stability_signal.cpp"
)

target_compile_options(tests PRIVATE /Zc:twoPhase-)

target_link_libraries(tests PRIVATE
    models_lib
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

set_target_properties(tests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${SRC_DIR}"
)

enable_testing()
add_test(NAME all_tests COMMAND tests WORKING_DIRECTORY "${SRC_DIR}")
