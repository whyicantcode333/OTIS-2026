# The lab 1 code (models, signals, simulator, plot.py) is NOT copied here: it is compiled
# straight from task_01/src. Only the new code of lab 2 lives in this directory.
get_filename_component(LAB1_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../task_01/src" ABSOLUTE)
if(NOT EXISTS "${LAB1_SRC}/models.cpp")
    message(FATAL_ERROR "Lab 1 sources were not found in ${LAB1_SRC}")
endif()

# Core library: everything except main(), so that unit tests can link against it.
add_library(otis_core STATIC
    ${LAB1_SRC}/models.cpp
    ${LAB1_SRC}/signals.cpp
    ${LAB1_SRC}/simulator.cpp
    stability.cpp
)
target_include_directories(otis_core PUBLIC ${LAB1_SRC} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(otis_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(otis_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(otis_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Code coverage (gcov/gcovr): -O0 so that line counts are exact.
if(OTIS_COVERAGE AND NOT MSVC)
    target_compile_options(otis_core PUBLIC --coverage -O0 -g)
    target_link_options(otis_core PUBLIC --coverage)
endif()

add_executable(otis_variant2_lab2 main.cpp)
target_link_libraries(otis_variant2_lab2 PRIVATE otis_core)
if(MSVC)
    target_compile_options(otis_variant2_lab2 PRIVATE /W4 /permissive-)
else()
    target_compile_options(otis_variant2_lab2 PRIVATE -Wall -Wextra -Wpedantic)
endif()        

# Make the plotting script of lab 1 available beside the executable.
add_custom_command(TARGET otis_variant2_lab2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LAB1_SRC}/plot.py
            $<TARGET_FILE_DIR:otis_variant2_lab2>/plot.py
    COMMENT "Copying plot.py next to the executable")
