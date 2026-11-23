cmake_minimum_required(VERSION 3.20)

project(task_02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TASK01_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../task_01/src")

# Библиотека объединяет исходники моделей из task_01 и новые модули из task_02 без копирования файлов
add_library(models_lib
    ${TASK01_SRC_DIR}/Model1_10.cpp
    ${TASK01_SRC_DIR}/Model2_2.cpp
    ${TASK01_SRC_DIR}/Model3_4.cpp
    ${TASK01_SRC_DIR}/InputSignals.cpp
    src/StabilityAnalyzer.cpp
    src/Simulation.cpp
)

target_include_directories(models_lib PUBLIC
    ${TASK01_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(models_lib PRIVATE /W4)
else()
    target_compile_options(models_lib PRIVATE -Wall -Wextra -Wpedantic --coverage)
    target_link_options(models_lib PUBLIC --coverage)
endif()

# Основная программа
add_executable(task_02
    src/main.cpp
)
target_link_libraries(task_02 PRIVATE models_lib)

# Модульные тесты
enable_testing()

add_executable(unit_tests
    test/test_models.cpp
)
target_link_libraries(unit_tests PRIVATE
    models_lib
    GTest::gtest_main
    GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)