#include "database.h"
#include "user.h"
#include "menu.h"

#include <iostream>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    Database db("school.db");
    if (!db.connect()) {
        std::cerr << "❌ Ошибка подключения к БД\n";
        return 1;
    }

    if (!db.initialize()) {
        std::cerr << "❌ Ошибка инициализации БД\n";
        return 1;
    }

    // Проверяем, пустое ли расписание (первая инициализация новой БД)
    bool scheduleEmpty = true;
    if (!db.isScheduleEmpty(scheduleEmpty)) {
        std::cerr << "❌ Не удалось проверить расписание\n";
        return 1;
    }

    if (scheduleEmpty) {
        std::cout << "\n[+] Первая инициализация: заливаем демо-данные и расписание...\n";

        if (!db.initializeDemoData()) {
            std::cerr << "❌ Ошибка заполнения демо-данных\n";
            return 1;
        }

        // 📅 Загрузка расписания 4 групп
        std::cout << "\n📅 ЗАГРУЗКА РАСПИСАНИЯ\n";
        std::vector<std::pair<int, std::string>> schedules = {
            {1, "schedule_420601_newest.sql"},
            {2, "schedule_420602_newest.sql"},
            {3, "schedule_420603_newest.sql"},
            {4, "schedule_420604_newest.sql"}
        };

        int successful = 0;
        for (const auto& [groupId, filename] : schedules) {
            std::cout << "📂 Загрузка расписания для группы " << (420600 + groupId) << "...\n";
            if (db.loadGroupSchedule(groupId, filename)) {
                std::cout << "✓ Расписание загружено из файла: " << filename << "\n";
                successful++;
            } else {
                std::cerr << "❌ Ошибка при загрузке файла: " << filename << "\n";
            }
        }

        // 🔍 Проверка количества пар
        std::cout << "\n🔍 ПРОВЕРКА ЗАГРУЗКИ\n";
        sqlite3_stmt* stmt = nullptr;
        const std::string query = "SELECT COUNT(*) FROM schedule";
        if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int total = sqlite3_column_int(stmt, 0);
                std::cout << "✓ Всего пар в расписании: " << total << "\n";
                if (total > 0) {
                    std::cout << "✅ РАСПИСАНИЕ УСПЕШНО ЗАГРУЖЕНО! ("
                              << successful << "/4 файлов)\n";
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

    // 🔐 Основной цикл авторизации
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
