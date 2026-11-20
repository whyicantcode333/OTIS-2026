cmake_minimum_required(VERSION 3.10)
project(task01_variant3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The executable always lands in <build>/bin (for any generator / configuration)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_BINARY_DIR}/bin")
endforeach()

add_executable(task01 main.cpp models.h signals.h)

# plot.py is copied next to the executable - the program starts it automatically
add_custom_command(TARGET task01 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/plot.py" "$<TARGET_FILE_DIR:task01>"
    COMMENT "Copying plot.py next to task01")
