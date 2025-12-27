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
        statistics.cpp
        services/student_service.cpp
        services/d1_randomizer.cpp

        third_party/sqlite/sqlite3.c
)

set(BACKEND_HEADERS
        database.h
        statistics.h
        core/result.h
        services/student_service.h
        services/d1_randomizer.h

        config.h
)

# ===== GUI-only (Qt UI) =====
set(GUI_SOURCES
        main.cpp
        loginwindow.cpp
        studentwindow_core.cpp
        studentwindow.cpp
        teacherwindow_core.cpp
        teacherwindow_schedule.cpp
        teacherwindow_journal.cpp
        teacherwindow_stats.cpp
        adminwindow_core.cpp
        adminwindow.cpp
        adminwindow_users.cpp
        adminwindow_schedule.cpp
        adminwindow_d1.cpp

        ui/util/AppEvents.cpp

        ui/style/ThemeManager.cpp
        ui/widgets/ThemeToggleWidget.cpp
        ui/widgets/PeriodSelectorWidget.cpp
        ui/widgets/LessonCardWidget.cpp
        ui/widgets/WeekGridScheduleWidget.cpp
        ui/windows/TeacherScheduleViewer.cpp
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
        ui/windows/TeacherScheduleViewer.h
        ui/pages/StudentSchedulePage.h
        ui/pages/StudentGradesPage.h
        ui/pages/StudentAbsencesPage.h
        ui/models/WeekSelection.h
        ui/util/TableWidgetStyle.h
        ui/util/UiStyle.h
        ui/util/AppEvents.h
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

if (MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set_property(TARGET app_gui PROPERTY AUTOMOC_COMPILER_PREDEFINES OFF)
    target_compile_options(app_gui PRIVATE -fdiagnostics-color=never)
endif()

