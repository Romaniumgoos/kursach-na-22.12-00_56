cmake_minimum_required(VERSION 3.31)
project(untitled)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# ===== Backend (общий код) =====
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
        database.h
        admin.h
        teacher.h
        student.h
        user.h
        statistics.h
        scholarship.h
        menu.h

        AdminService.h
        core/result.h
        services/student_service.h

        config.h
)

# ===== GUI-only (Qt UI) =====
set(GUI_SOURCES
        main.cpp
        loginwindow.cpp
        studentwindow.cpp
        teacherwindow.cpp
        adminwindow.cpp

        ui/style/ThemeManager.cpp
        ui/widgets/ThemeToggleWidget.cpp
        ui/widgets/PeriodSelectorWidget.cpp
        ui/widgets/LessonCardWidget.cpp
        ui/widgets/WeekGridScheduleWidget.cpp
        ui/pages/StudentSchedulePage.cpp
        ui/pages/StudentGradesPage.cpp
        ui/pages/StudentAbsencesPage.cpp
)

set(GUI_HEADERS
        loginwindow.h
        studentwindow.h
        teacherwindow.h
        adminwindow.h

        ui/style/ThemeManager.h
        ui/widgets/ThemeToggleWidget.h
        ui/widgets/PeriodSelectorWidget.h
        ui/widgets/LessonCardWidget.h
        ui/widgets/WeekGridScheduleWidget.h
        ui/pages/StudentSchedulePage.h
        ui/pages/StudentGradesPage.h
        ui/pages/StudentAbsencesPage.h
        ui/models/WeekSelection.h
        ui/util/TableWidgetStyle.h
        ui/util/UiStyle.h
)

add_executable(app_gui WIN32
        ${GUI_SOURCES}
        ${GUI_HEADERS}
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

# ===== Console executable =====
add_executable(app_console
        main_console.cpp
        ${BACKEND_SOURCES}
        ${BACKEND_HEADERS}
)

target_include_directories(app_console PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)
