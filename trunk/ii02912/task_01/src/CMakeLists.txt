cmake_minimum_required(VERSION 3.10)
project(Lab01Models_ii02912 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab01_ii02912 main.cpp Model.hpp Input.hpp)
set_target_properties(lab01_ii02912 PROPERTIES OUTPUT_NAME lab01)
target_compile_definitions(lab01_ii02912 PRIVATE PLOT_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/plot.py")
if(MSVC)
  target_compile_options(lab01_ii02912 PRIVATE /W4 /utf-8)
else()
  target_compile_options(lab01_ii02912 PRIVATE -Wall -Wextra)
endif()
