cmake_minimum_required(VERSION 3.31)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
        main.cpp
        database.cpp
        admin.cpp
        teacher.cpp
        student.cpp
        user.cpp
        third_party/sqlite/sqlite3.c
        statistics.h
        statistics.cpp
        scholarship.h
        scholarship.cpp
        menu.h
        menu.cpp
        AdminService.h
        AdminService.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        third_party/sqlite
)
