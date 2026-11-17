cmake_minimum_required(VERSION 3.14)
project(task_02_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Автоматическое скачивание Google Test через FetchContent ---
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Для Windows: не переопределять стандартные флаги MSVC
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# --- Подключаем исходники и тесты ---
add_subdirectory(src)
add_subdirectory(test)

# --- Включаем CTest ---
enable_testing()