cmake_minimum_required(VERSION 3.20)

set(TASK01_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../task_01/src")

add_executable(ii03013_task_02
    main.cpp
    StabilityAnalyzer.cpp
    Simulation.cpp
    ${TASK01_SRC_DIR}/Model1_10.cpp
    ${TASK01_SRC_DIR}/Model2_2.cpp
    ${TASK01_SRC_DIR}/Model3_4.cpp
    ${TASK01_SRC_DIR}/InputSignals.cpp
)

target_include_directories(ii03013_task_02 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TASK01_SRC_DIR}
)