cmake_minimum_required(VERSION 3.15)

project(ii02909_task_02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Google Test через FetchContent ----
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Не даёт CMake пересобирать gtest каждый раз
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ---- Собираем основную программу ----
add_subdirectory(src)

# ---- Собираем тесты ----
add_subdirectory(test)