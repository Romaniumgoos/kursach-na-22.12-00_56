#cmake_minimum_required(VERSION 3.31)
#project(untitled)
#
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#
#add_executable(${PROJECT_NAME}
#        main.cpp
#        database.cpp
#        admin.cpp
#        teacher.cpp
#        student.cpp
#        user.cpp
#        third_party/sqlite/sqlite3.c
#        statistics.h
#        statistics.cpp
#        scholarship.h
#        scholarship.cpp
#        menu.h
#        menu.cpp
#        AdminService.h
#        AdminService.cpp
#        core/result.h
#        services/student_service.h
#        services/student_service.cpp
#        main.cpp
#)
#
#target_include_directories(${PROJECT_NAME} PRIVATE
#        ${CMAKE_CURRENT_SOURCE_DIR}
#        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
#)
cmake_minimum_required(VERSION 3.31)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# ===== Общие исходники (backend) =====
set(BACKEND_SOURCES
        database.cpp
        admin.cpp
        teacher.cpp
        student.cpp
        user.cpp
        statistics.cpp
        scholarship.cpp
        menu.cpp

        AdminService.cpp
        services/student_service.cpp

        third_party/sqlite/sqlite3.c
)

set(BACKEND_HEADERS
        statistics.h
        scholarship.h
        menu.h
        AdminService.h
        core/result.h
        services/student_service.h
        config.h

)

# ===== GUI executable (Qt) =====
add_executable(app_gui
        main.cpp
        loginwindow.h loginwindow.cpp
        studentwindow.h studentwindow.cpp
        teacherwindow.h teacherwindow.cpp
        adminwindow.h adminwindow.cpp
        ${BACKEND_SOURCES}
        ${BACKEND_HEADERS}
)

target_include_directories(app_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

target_link_libraries(app_gui PRIVATE
        Qt6::Core
        Qt6::Widgets
)

# ===== Console executable (старый режим) =====
add_executable(app_console
        main_console.cpp
        ${BACKEND_SOURCES}
        ${BACKEND_HEADERS}
)

target_include_directories(app_console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)



