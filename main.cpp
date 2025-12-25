#include <QApplication>

#include "config.h"
#include "database.h"
#include "loginwindow.h"
#include "ui/style/ThemeManager.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>
#include <memory>
 #include <utility>
 #include <vector>

static bool hasTable(sqlite3* db, const char* tableName)
{
    if (!db || !tableName) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_bind_text(stmt, 1, tableName, -1, SQLITE_TRANSIENT);
    const bool ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

static bool schemaLooksLikeSnakeCase(sqlite3* db)
{
    if (!db) return false;
    // Если есть snake_case таблицы и нет наших camel/sans-underscore аналогов.
    const bool hasCycleWeeksSnake = hasTable(db, "cycleweeks");
    const bool hasTeacherGroupsSnake = hasTable(db, "teachergroups");
    const bool hasTeacherSubjectsSnake = hasTable(db, "teachersubjects");

    const bool hasCycleweeks = hasTable(db, "cycleweeks");
    const bool hasTeacherGroups = hasTable(db, "teachergroups");
    const bool hasTeacherSubjects = hasTable(db, "teachersubjects");

    // Если есть cycle_weeks, но нет cycleweeks — это точно другой мир.
    if (hasCycleWeeksSnake && !hasCycleweeks) return true;

    // teacher_groups/teacher_subjects указывают на старую схему, если нет camelCase аналогов.
    Q_UNUSED(hasTeacherGroupsSnake);
    Q_UNUSED(hasTeacherSubjectsSnake);
    Q_UNUSED(hasTeacherGroups);
    Q_UNUSED(hasTeacherSubjects);

    if (hasTeacherGroupsSnake && !hasTeacherGroups) return true;
    if (hasTeacherSubjectsSnake && !hasTeacherSubjects) return true;

    return false;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Тема
    ThemeManager::instance()->applyTheme(&app);

    // БД используем ту же, что и консольная версия (PROJECT_ROOT\school.db)
    const QString dbPath = QDir(QString::fromStdString(PROJECT_ROOT)).filePath("school.db");

    std::unique_ptr<Database> db = std::make_unique<Database>(dbPath.toStdString());
    if (!db->connect()) return 1;
    if (!db->initialize()) return 1;

     std::cerr << "[DB] opened path: " << dbPath.toStdString() << "\n";

     bool scheduleEmpty = true;
     if (!db->isScheduleEmpty(scheduleEmpty)) {
         std::cerr << "[DB] isScheduleEmpty failed\n";
     }

     // Если БД пустая (обычно в Release), делаем ровно один раз демо-инициализацию и загрузку расписаний.
     if (scheduleEmpty) {
         std::cerr << "[DB] schedule is empty -> initializing demo data\n";
         if (!db->initializeDemoData()) {
             return 1;
         }

         const std::vector<std::pair<int, QString>> schedules = {
             {1, QDir(QString::fromStdString(PROJECT_ROOT)).filePath("schedule_420601_newest.sql")},
             {2, QDir(QString::fromStdString(PROJECT_ROOT)).filePath("schedule_420602_newest.sql")},
             {3, QDir(QString::fromStdString(PROJECT_ROOT)).filePath("schedule_420603_newest.sql")},
             {4, QDir(QString::fromStdString(PROJECT_ROOT)).filePath("schedule_420604_newest.sql")}
         };

         int successful = 0;
         for (const auto& p : schedules) {
             if (db->loadGroupSchedule(p.first, p.second.toStdString())) {
                 ++successful;
             } else {
                 std::cerr << "[DB] failed to load schedule file: " << p.second.toStdString() << "\n";
             }
         }

         std::cerr << "[DB] loaded schedules: " << successful << "/" << schedules.size() << "\n";
     }

     // Safe auth self-test: log current state; no destructive writes if DB already has users.
     {
         sqlite3_stmt* stmt = nullptr;
         int usersCount = -1;
         if (sqlite3_prepare_v2(db->getHandle(), "SELECT COUNT(*) FROM users;", -1, &stmt, nullptr) == SQLITE_OK) {
             if (sqlite3_step(stmt) == SQLITE_ROW) {
                 usersCount = sqlite3_column_int(stmt, 0);
             }
         }
         sqlite3_finalize(stmt);
         std::cerr << "[DB auth] users: " << usersCount << "\n";

         int id = 0;
         std::string name;
         std::string role;
         const bool ok = db->findUser("admin", "123", id, name, role);
         std::cerr << "[DB auth] findUser(admin/123): " << (ok ? "OK" : "FAIL")
                   << " id=" << id << " role=" << role << "\n";
     }

#ifdef QT_DEBUG
    db->dumpDbStats();
    db->dumpSchemaAndCounts();

    // Если рядом с exe лежит "чужой" school.db со snake_case таблицами (cycle_weeks, ...),
    // то не трогаем его, а в debug переключаемся на отдельный файл с нашей схемой.
    if (schemaLooksLikeSnakeCase(db->getHandle())) {
        const QString altPath = QDir(QCoreApplication::applicationDirPath()).filePath("school_qt.db");
        std::cerr << "[DB] schema mismatch detected. Switching to: " << altPath.toStdString() << "\n";

        db = std::make_unique<Database>(altPath.toStdString());
        if (!db->connect()) return 1;
        if (!db->initialize()) return 1;

        db->dumpDbStats();
        db->dumpSchemaAndCounts();
    }

    // End-to-end self-test (DB -> schedule rows), no UI.
    int testStudentId = 0;
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db->getHandle(),
                               "SELECT id FROM users WHERE role='student' LIMIT 1;",
                               -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                testStudentId = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
    }

    if (testStudentId > 0) {
        int groupId = 0;
        int subgroup = 0;
        if (db->getStudentGroupAndSubgroup(testStudentId, groupId, subgroup)) {
            int total = 0;
            for (int w = 1; w <= 4; ++w) {
                for (int d = 0; d <= 5; ++d) {
                    std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> rows;
                    if (db->getScheduleForGroup(groupId, d, w, rows)) {
                        total += static_cast<int>(rows.size());
                    }
                }
            }
            std::cerr << "[DB self-test] studentId=" << testStudentId
                      << " groupId=" << groupId
                      << " subgroup=" << subgroup
                      << " totalRows=" << total << "\n";
        } else {
            std::cerr << "[DB self-test] studentId=" << testStudentId << " has no group/subgroup\n";
        }
    } else {
        std::cerr << "[DB self-test] no student found\n";
    }
#endif

    LoginWindow window(db.get());
    window.show();

    return app.exec();
}
