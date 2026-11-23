cmake_minimum_required(VERSION 3.16)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0000)
    cmake_policy(SET CMP0000 NEW)
endif()

project(OTIS_Task02 LANGUAGES CXX)

enable_testing()
add_subdirectory(src)