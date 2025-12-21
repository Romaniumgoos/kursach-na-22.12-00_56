# Find GTest
find_package(GTest REQUIRED)
include(GoogleTest)

# Add test executable
add_executable(test_schedule_refactoring 
    test_schedule_refactoring.cpp
    ${CMAKE_SOURCE_DIR}/database.cpp
    ${CMAKE_SOURCE_DIR}/database.h
)

# Link with GTest and SQLite3
target_link_libraries(test_schedule_refactoring 
    PRIVATE 
    GTest::GTest 
    GTest::Main
    sqlite3
)

# Include directories
target_include_directories(test_schedule_refactoring 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

# Add test
gtest_discover_tests(test_schedule_refactoring)
