project(Task02_Models)

add_library(ii02906_models
    LinearModel.cpp
    NonLinearModel.cpp
    DiffEqModel.cpp
)

target_include_directories(ii02906_models PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})