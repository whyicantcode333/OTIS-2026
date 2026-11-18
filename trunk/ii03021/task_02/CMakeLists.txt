cmake_minimum_required(VERSION 3.18)
project(OTIS_Variant2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OTIS_BUILD_TESTS "Build unit tests (Google Test / Google Mock)" ON)
option(OTIS_COVERAGE "Instrument the code for gcov/gcovr" OFF)

add_subdirectory(src)

if(OTIS_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(test)
endif()
