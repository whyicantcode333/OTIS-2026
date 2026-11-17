# Исходники моделей (библиотека, которую будут использовать тесты)
add_library(models_lib STATIC
    stability.cpp
    # stability.hpp — заголовочный файл, компилировать не нужно
)

target_include_directories(models_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})