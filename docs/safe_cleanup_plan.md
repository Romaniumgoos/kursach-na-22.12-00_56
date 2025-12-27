# Safe cleanup plan (NO-COMMIT / NO-PUSH)

## 1) GUI build facts (authoritative)

### GUI target name

`app_gui`

Source: `CMakeLists.txt`:

- `add_executable(app_gui WIN32 ... ${GUI_SOURCES} ${GUI_HEADERS} ${BACKEND_SOURCES} ${BACKEND_HEADERS})`

### Qt modules linked

- `Qt6::Core`
- `Qt6::Widgets`

Source: `CMakeLists.txt`:

- `find_package(Qt6 REQUIRED COMPONENTS Core Widgets)`
- `target_link_libraries(app_gui PRIVATE Qt6::Core Qt6::Widgets)`

Note: `Qt6::Gui` and `Qt6::Sql` are NOT linked explicitly in `CMakeLists.txt`.

### GUI entrypoint

- `main.cpp` (contains `int main(int argc, char* argv[])` and creates `QApplication`)

Source: `main.cpp`.

## 2) GUI sources list (DO NOT CHANGE)

Definition per CMake: everything that is passed to `app_gui` via `${GUI_SOURCES} ${GUI_HEADERS} ${BACKEND_SOURCES} ${BACKEND_HEADERS}`.

### GUI_SOURCES

- `main.cpp`
- `loginwindow.cpp`
- `studentwindow.cpp`
- `teacherwindow.cpp`
- `adminwindow.cpp`
- `ui/util/AppEvents.cpp`
- `ui/style/ThemeManager.cpp`
- `ui/widgets/ThemeToggleWidget.cpp`
- `ui/widgets/PeriodSelectorWidget.cpp`
- `ui/widgets/LessonCardWidget.cpp`
- `ui/widgets/WeekGridScheduleWidget.cpp`
- `ui/windows/TeacherScheduleViewer.cpp`
- `ui/pages/StudentSchedulePage.cpp`
- `ui/pages/StudentGradesPage.cpp`
- `ui/pages/StudentAbsencesPage.cpp`

### GUI_HEADERS

- `loginwindow.h`
- `studentwindow.h`
- `teacherwindow.h`
- `adminwindow.h`
- `ui/style/ThemeManager.h`
- `ui/widgets/ThemeToggleWidget.h`
- `ui/widgets/PeriodSelectorWidget.h`
- `ui/widgets/LessonCardWidget.h`
- `ui/widgets/WeekGridScheduleWidget.h`
- `ui/windows/TeacherScheduleViewer.h`
- `ui/pages/StudentSchedulePage.h`
- `ui/pages/StudentGradesPage.h`
- `ui/pages/StudentAbsencesPage.h`
- `ui/models/WeekSelection.h`
- `ui/util/TableWidgetStyle.h`
- `ui/util/UiStyle.h`
- `ui/util/AppEvents.h`

### BACKEND_SOURCES

- `database.cpp`
- `admin.cpp`
- `teacher.cpp`
- `student.cpp`
- `user.cpp`
- `statistics.cpp`
- `scholarship.cpp`
- `menu.cpp`
- `AdminService.cpp`
- `services/student_service.cpp`
- `services/d1_randomizer.cpp`
- `third_party/sqlite/sqlite3.c`

### BACKEND_HEADERS

- `database.h`
- `admin.h`
- `teacher.h`
- `student.h`
- `user.h`
- `statistics.h`
- `scholarship.h`
- `menu.h`
- `AdminService.h`
- `core/result.h`
- `services/student_service.h`
- `services/d1_randomizer.h`
- `config.h`

## 3) Referenced-by-GUI (reachable includes)

### Direct includes from GUI sources (examples / key edges)

- `main.cpp` includes:
  - `config.h`
  - `database.h`
  - `loginwindow.h`
  - `ui/style/ThemeManager.h`
- `loginwindow.cpp` includes:
  - `loginwindow.h`
  - `studentwindow.h`
  - `teacherwindow.h`
  - `adminwindow.h`
- `adminwindow.cpp` includes:
  - `adminwindow.h`
  - `ui/util/AppEvents.h`
  - `ui/util/UiStyle.h`
  - `ui/widgets/ThemeToggleWidget.h`
  - `ui/widgets/PeriodSelectorWidget.h`
  - `ui/widgets/WeekGridScheduleWidget.h`
  - `services/d1_randomizer.h`
  - `loginwindow.h`
- `ui/pages/StudentGradesPage.cpp` includes:
  - `ui/pages/StudentGradesPage.h`
  - `database.h`
  - `statistics.h`
  - `ui/util/UiStyle.h`

### Closure (what реально участвует)

Because `app_gui` links *all* `${BACKEND_SOURCES}` and `${BACKEND_HEADERS}`, the following backend files are part of the GUI build **even if they are primarily “console logic”**:

- `admin.cpp/.h`, `teacher.cpp/.h`, `student.cpp/.h`, `user.cpp/.h`, `menu.cpp/.h` and related headers.

So they are all **Referenced-by-GUI** by definition of the current CMake target composition.

Additionally, GUI-side includes pull these backend headers directly:

- `database.h` is directly included by `main.cpp`, `loginwindow.h`, `adminwindow.h`, `studentwindow.h`, `teacherwindow.h`, and multiple UI pages/widgets.

## 4) Cleanup candidates classification

### A) Safe delete artifacts (safe to delete immediately)

These are build artifacts / logs. They do not participate in the source build and can be removed without affecting `app_gui` compilation.

Directories:

- `cmake-build-debug/`
- `cmake-build-qt-mingw/`
- `cmake-build-qt-mingw-mingw/`
- `cmake-build-qt-mingw-ninja/`
- `cmake-build-qt-mingw-ninja2/`
- `cmake-build-qt-ninja-qtmingw/`

Top-level and build log files (examples from repo root):

- `*_err.txt`:
  - `build_err.txt`
  - `gui_after_polish_err.txt`
  - `gui_build_err.txt`
  - `make_one_err.txt`
  - `make_tw_err.txt`
  - `mc_cmd_err.txt`
  - `moc_always_err.txt`
  - `moc_compile_err.txt`
  - `moc_err_err.txt`
  - `teacher_compile_err.txt`
  - `teacher_gpp_err.txt`
- `*_out.txt`:
  - `build_out.txt`
  - `gui_after_polish_out.txt`
  - `gui_build_out.txt`
  - `make_one_out.txt`
  - `make_tw_out.txt`
  - `mc_cmd_out.txt`
  - `moc_always_out.txt`
  - `moc_compile_out.txt`
  - `moc_err_out.txt`
  - `teacher_compile_out.txt`
  - `teacher_gpp_out.txt`
- `*build*.txt`:
  - `build_make_root.txt`
  - `build_make_snip.txt`
  - `build_verbose.txt`
  - `build_verbose2.txt`
  - `console_build_log.txt`
- `*verbose*.txt`:
  - `abi_verbose.txt`
  - `build_verbose.txt`
  - `build_verbose2.txt`

Pattern-specific artifacts present in repo root:

- `gpp_*`:
  - `gpp_err_root.txt`
  - `gpp_out_root.txt`
  - `gpp_rsp_err_root.txt`
  - `gpp_rsp_out_root.txt`
- `moc_*`:
  - `moc_always_err.txt`, `moc_always_out.txt`
  - `moc_compile_err.txt`, `moc_compile_out.txt`
  - `moc_err_err.txt`, `moc_err_out.txt`
- `make_*`:
  - `make_one_err.txt`, `make_one_out.txt`
  - `make_tw_err.txt`, `make_tw_out.txt`
- `mc_cmd_*`:
  - `mc_cmd_err.txt`, `mc_cmd_out.txt`
- `*_link_*`:
  - `app_console_link_root.txt`
- `app_gui_*`:
  - `app_gui_build_make_cur.make`
  - `app_gui_build_root.make`
  - `app_gui_flags_root.make`
  - `app_gui_moc_lines.txt`
- `app_console_*`:
  - `app_console_flags_root.make`
  - `app_console_includes_CXX.rsp.txt`
  - `app_console_includes_root.rsp`
  - `app_console_link_root.txt`
- `*.rsp / *.rsp.txt`:
  - `app_console_includes_root.rsp`
  - `app_console_includes_CXX.rsp.txt`
- `*_flags.make`:
  - `app_gui_flags_root.make`
  - `app_console_flags_root.make`
- `*_includes*`:
  - `app_console_includes_CXX.rsp.txt`
  - `app_console_includes_root.rsp`

### B) Console-only delete candidates (delete later, only with proofs)

Candidate:

- `main_console.cpp`

Proofs:

- Not in GUI sources:
  - In `CMakeLists.txt`, `main_console.cpp` appears only under `add_executable(app_console ...)`.
  - It is NOT listed in `GUI_SOURCES`, `GUI_HEADERS`, `BACKEND_SOURCES`, `BACKEND_HEADERS`.
- Not referenced-by-GUI:
  - No `#include "main_console.cpp"` anywhere (entrypoint file).
  - GUI-side files include backend headers (`database.h`, etc.), but none include or depend on `main_console.cpp`.

Notes:

- Files like `admin.cpp/.h`, `teacher.cpp/.h`, `student.cpp/.h`, `user.cpp/.h`, `menu.cpp/.h` are NOT console-only in the current build, because they are included in `${BACKEND_SOURCES}` / `${BACKEND_HEADERS}` and therefore compiled into `app_gui`.

### C) Old/backup candidates

Candidate:

- `olddatabase.cpp`

Proofs:

- Not in GUI sources:
  - `olddatabase.cpp` is NOT listed in `BACKEND_SOURCES` / `GUI_SOURCES` in `CMakeLists.txt`.
- Not referenced-by-GUI:
  - Repository search for token `olddatabase` returns no references (no includes, no symbol usage).

### D) Demo-data

Present in repo root:

- `school.db`
- `schoolbackup.db` (also appears inside `cmake-build-qt-mingw-mingw/` as a build dir artifact)
- `schedule_420601_newest.sql`
- `schedule_420602_newest.sql`
- `schedule_420603_newest.sql`
- `schedule_420604_newest.sql`

Decision: **Variant A (keep demo data in repo for now)**.

Reason:

- `main.cpp` in GUI explicitly loads `school.db` from `PROJECT_ROOT` and, if schedule is empty, attempts to load the `schedule_42060x_newest.sql` files from `PROJECT_ROOT`.
- Removing these files from git without changing code would make “first run / empty DB” flow fail.

Recommended follow-up (future step, separate change):

- Move demo assets into `data/` (or `resources/`) and adjust `main.cpp` paths accordingly, or embed them via Qt resources (`.qrc`).

## What to delete on step 2 (ONLY category A)

- `cmake-build-debug/`
- `cmake-build-qt-mingw/`
- `cmake-build-qt-mingw-mingw/`
- `cmake-build-qt-mingw-ninja/`
- `cmake-build-qt-mingw-ninja2/`
- `cmake-build-qt-ninja-qtmingw/`
- All files matching:
  - `*_err.txt`, `*_out.txt`
  - `*build*.txt`, `*compile*.txt`, `*verbose*.txt`
  - `gpp_*`, `moc_*`, `make_*`, `mc_cmd_*`, `*_link_*`
  - `app_gui_*`, `app_console_*`
  - `*.rsp`, `*.rsp.txt`, `*_flags.make`, `*_includes*`
