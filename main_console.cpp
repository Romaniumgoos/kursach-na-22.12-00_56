#include "database.h"
#include "user.h"
#include "menu.h"
#include "config.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// sqlite symbols (sqlite3_stmt, SQLITE_OK, etc.)
#include "sqlite3.h"

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    Database db(PROJECT_ROOT + "\\school.db");
    if (!db.connect()) {
        std::cerr << "❌ Ошибка подключения к БД\n";
        return 1;
    }

    if (!db.initialize()) {
        std::cerr << "❌ Ошибка инициализации БД\n";
        return 1;
    }

    bool scheduleEmpty = true;
    if (!db.isScheduleEmpty(scheduleEmpty)) {
        std::cerr << "❌ Не удалось проверить расписание\n";
        return 1;
    }

    int successful = 0; // объявляем ЗДЕСЬ, чтобы видеть и ниже (для вывода)

    if (scheduleEmpty) {
        std::cout << "\n[+] Первая инициализация: заливаем демо-данные и расписание...\n";

        if (!db.initializeDemoData()) {
            std::cerr << "❌ Ошибка заполнения демо-данных\n";
            return 1;
        }

        std::cout << "\n📅 ЗАГРУЗКА РАСПИСАНИЯ\n";
        const std::vector<std::pair<int, std::string>> schedules = {
            {1, PROJECT_ROOT + "\\schedule_420601_newest.sql"},
            {2, PROJECT_ROOT + "\\schedule_420602_newest.sql"},
            {3, PROJECT_ROOT + "\\schedule_420603_newest.sql"},
            {4, PROJECT_ROOT + "\\schedule_420604_newest.sql"}
        };

        for (const auto& [groupId, filename] : schedules) {
            if (db.loadGroupSchedule(groupId, filename)) {
                ++successful;
            } else {
                std::cerr << "❌ Ошибка при загрузке файла: " << filename << "\n";
            }
        }

        // Проверка дублей общих лекций (group_id=0) — не обязательна, но полезна
        {
            sqlite3_stmt* dupStmt = nullptr;
            const char* dupSql =
                "SELECT weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lessontype, COUNT(*) "
                "FROM schedule "
                "WHERE groupid = 0 AND lessontype = 'ЛК' "
                "GROUP BY weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lessontype "
                "HAVING COUNT(*) > 1;";

            if (sqlite3_prepare_v2(db.getHandle(), dupSql, -1, &dupStmt, nullptr) == SQLITE_OK) {
                bool hasDup = false;
                while (sqlite3_step(dupStmt) == SQLITE_ROW) {
                    if (!hasDup) {
                        std::cerr << "⚠ WARNING: найдены дубли общих лекций (group_id=0)\n";
                        hasDup = true;
                    }

                    const int weekday = sqlite3_column_int(dupStmt, 0);
                    const int lessonNumber = sqlite3_column_int(dupStmt, 1);
                    const int weekOfCycle = sqlite3_column_int(dupStmt, 2);
                    const int subjectId = sqlite3_column_int(dupStmt, 3);
                    const int teacherId = sqlite3_column_int(dupStmt, 4);
                    const unsigned char* roomText = sqlite3_column_text(dupStmt, 5);
                    const unsigned char* lessonTypeText = sqlite3_column_text(dupStmt, 6);
                    const int cnt = sqlite3_column_int(dupStmt, 7);

                    const std::string room = roomText ? reinterpret_cast<const char*>(roomText) : "";
                    const std::string lessonType = lessonTypeText ? reinterpret_cast<const char*>(lessonTypeText) : "";

                    std::cerr
                        << "  weekday=" << weekday
                        << ", lessonnumber=" << lessonNumber
                        << ", weekofcycle=" << weekOfCycle
                        << ", subjectid=" << subjectId
                        << ", teacherid=" << teacherId
                        << ", room='" << room << "'"
                        << ", lessontype='" << lessonType << "'"
                        << ", count=" << cnt << "\n";
                }
                sqlite3_finalize(dupStmt);
            } else {
                std::cerr << "❌ Не удалось выполнить проверку дублей лекций\n";
            }
        }

        // Проверка количества пар
        std::cout << "\n🔍 ПРОВЕРКА ЗАГРУЗКИ\n";
        sqlite3_stmt* stmt = nullptr;
        const char* query = "SELECT COUNT(*) FROM schedule";
        if (sqlite3_prepare_v2(db.getHandle(), query, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int total = sqlite3_column_int(stmt, 0);
                std::cout << "✓ Всего пар в расписании: " << total << "\n";
                if (total > 0) {
                    std::cout << "✅ РАСПИСАНИЕ УСПЕШНО ЗАГРУЖЕНО! (" << successful << "/4 файлов)\n";
                } else {
                    std::cout << "❌ РАСПИСАНИЕ НЕ ЗАГРУЖЕНО!\n";
                }
            }
            sqlite3_finalize(stmt);
        } else {
            std::cerr << "❌ Не удалось выполнить проверочный запрос COUNT(*)\n";
        }
    } else {
        std::cout << "\n[+] Найдена существующая БД, демо-данные и расписание не перезаполняем.\n";
    }

    // Основной цикл авторизации
    while (true) {
        std::string username, password;
        std::cout << "\n=== Авторизация ===\n";
        std::cout << "Логин (или 'exit' для выхода): ";
        std::cin >> username;
        if (username == "exit") {
            std::cout << "До свидания!\n";
            break;
        }

        std::cout << "Пароль: ";
        std::cin >> password;

        auto user = User::authenticate(db, username, password);
        if (!user) {
            std::cerr << "❌ Неверный логин или пароль.\n";
            continue;
        }

        std::cout << "\n✓ Добро пожаловать, " << user->getName()
                  << " [" << user->getRole() << "]\n\n";

        user->displayMenu(db);
    }

    std::cout << "Программа завершена.\n";
    db.disconnect();
    return 0;
}
