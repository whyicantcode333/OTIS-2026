# Тестовый исполняемый файл
add_executable(task_02_tests
    test_stability.cpp
)

# Подключаем библиотеку моделей и Google Test
target_link_libraries(task_02_tests PRIVATE
    models_lib
    GTest::gtest
    GTest::gtest_main
)

# Включаем тест в CTest
include(GoogleTest)
gtest_discover_tests(task_02_tests)