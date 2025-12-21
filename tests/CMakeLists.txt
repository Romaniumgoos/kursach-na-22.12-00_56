# Настройка компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск зависимостей
find_package(GTest REQUIRED)
include(GoogleTest)

# Добавляем заголовочные файлы
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/sqlite
)

# Список исходных файлов для тестов
set(TEST_SOURCES
    test_schedule_refactoring.cpp
    ${CMAKE_SOURCE_DIR}/database.cpp
    ${CMAKE_SOURCE_DIR}/database.h
    ${CMAKE_SOURCE_DIR}/teacher.cpp
    ${CMAKE_SOURCE_DIR}/teacher.h
    ${CMAKE_SOURCE_DIR}/user.h
)

# Создаем исполняемый файл тестов
add_executable(schedule_tests ${TEST_SOURCES})

# Подключаем зависимости
target_link_libraries(schedule_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    sqlite3
)

# Добавляем тесты
gtest_discover_tests(schedule_tests)
