#include "admin.h"
#include "database.h"
#include "statistics.h"
#include "scholarship.h"

#include <iostream>
#include <limits>
#include <tuple>
#include <vector>
#include "menu.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// ===== КОНСТРУКТОР =====
Admin::Admin(int admin_id,
             const std::string& username,
             const std::string& name,
             Database* db)
    : User(admin_id, username, name, "admin"),
      admin_id_(admin_id),
      db_(db) {}


// ===== НОВЫЕ ФУНКЦИИ РАСПИСАНИЯ =====

bool Admin::viewScheduleForGroup(int group_Id, int sub_group, int week_of_cycle) {
    if (!db_ || !db_->isConnected()) {
        std::cerr << "[✗] БД не инициализирована\n";
        return false;
    }

    sqlite3* raw_db = db_->getHandle();
    const char* sql = R"(
        SELECT
            sch.id,          -- 0
            sch.weekday,     -- 1
            sch.lesson_number,-- 2
            subj.name,       -- 3
            u.name,          -- 4
            sch.room,        -- 5
            sch.sub_group,    -- 6
            sch.lesson_type  -- 7
        FROM schedule sch
        JOIN subjects subj ON sch.subject_id = subj.id
        JOIN users u ON sch.teacher_id = u.id
        WHERE (sch.group_id = ? OR sch.group_id = 0)
          AND sch.week_of_cycle = ?
        ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] prepare error: " << sqlite3_errmsg(raw_db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_Id);
    sqlite3_bind_int(stmt, 2, week_of_cycle);

    const char* dayName[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
    static const char* kPairTimes[] = {
        "08:30-09:55",
        "10:05-11:30",
        "12:00-13:25",
        "13:35-15:00",
        "15:30-16:55",
        "17:05-18:30"
    };

    std::cout << "\n╔═══ РАСПИСАНИЕ ГРУППЫ " << group_Id
              << " (неделя " << week_of_cycle << ") ═══╗\n";

    bool found = false;
    int currentDay = -1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId = sqlite3_column_int(stmt, 0);
        int weekday    = sqlite3_column_int(stmt, 1);
        int lessonNumber = sqlite3_column_int(stmt, 2);
        const char* subj  = (const char*)sqlite3_column_text(stmt, 3);
        const char* teach = (const char*)sqlite3_column_text(stmt, 4);
        const char* room  = (const char*)sqlite3_column_text(stmt, 5);
        int subgrp        = sqlite3_column_int(stmt, 6);
        const char* ltype = (const char*)sqlite3_column_text(stmt, 7);

        if (weekday < 0 || weekday > 5) continue;

        // фильтр по подгруппе
        if (sub_group != 0 && subgrp != 0 && subgrp != sub_group) {
            continue;
        }

        if (weekday != currentDay) {
            currentDay = weekday;

            std::string dateISO;
            std::string dateLabel;
            if (db_->getDateForWeekday(week_of_cycle, weekday, dateISO)) {
                if (dateISO.size() == 10) {
                    dateLabel = dateISO.substr(8, 2) + "-" + dateISO.substr(5, 2);
                }
            }

            if (!dateLabel.empty()) {
                std::cout << "\n[" << dayName[weekday] << "] (" << dateLabel << ")\n";
            } else {
                std::cout << "\n[" << dayName[weekday] << "]\n";
            }
        }

        found = true;

        std::string subjectText = subj ? subj : "";
        if (subgrp == 1) subjectText += " (подгр. 1)";
        else if (subgrp == 2) subjectText += " (подгр. 2)";

        std::string typeLabel;
        if (ltype && *ltype) {
            typeLabel = " [" + std::string(ltype) + "]";
        }

        std::cout << " ID " << scheduleId << " | Пара " << lessonNumber;
        if (lessonNumber >= 1 && lessonNumber <= 6) {
            std::cout << " (" << kPairTimes[lessonNumber - 1] << ")";
        }

        std::cout << " | " << subjectText << typeLabel
                  << " | " << (teach ? teach : "")
                  << " | ауд. " << (room ? room : "") << "\n";
    }

    if (!found) {
        std::cout << "║ (нет записей в расписании)\n";
    }

    std::cout << "╚════════════════════════════════════════╝\n";
    sqlite3_finalize(stmt);
    return true;
}


bool Admin::viewScheduleForTeacher(int teacherId, int week_of_cycle) {
    if (!db_ || !db_->isConnected()) {
        std::cerr << "[✗] Нет соединения с БД.\n";
        return false;
    }

    sqlite3* raw_db = db_->getHandle();
    const char* sql = R"(
        SELECT
            sch.weekday,        -- 0
            sch.lesson_number,   -- 1
            g.name       AS group_name, -- 2
            subj.name    AS subject,    -- 3
            sch.room,                -- 4
            sch.sub_group,            -- 5
            sch.lesson_type          -- 6
        FROM schedule sch
        JOIN subjects subj ON sch.subject_id = subj.id
        JOIN groups   g    ON sch.group_id  = g.id
        WHERE sch.teacher_id   = ?
          AND sch.week_of_cycle = ?
        ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] prepare error: " << sqlite3_errmsg(raw_db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, week_of_cycle);

    const char* dayName[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
    const char* pairTimes[] = {
        "08:30-09:55",
        "10:05-11:30",
        "12:00-13:25",
        "13:35-15:00",
        "15:30-16:55",
        "17:05-18:30"
    };


    std::cout << "\n╔═══ РАСПИСАНИЕ ПРЕПОДАВАТЕЛЯ ID "
              << teacherId << " (неделя " << week_of_cycle << ") ═══╗\n";

    bool found = false;
    int currentDay = -1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        int weekday      = sqlite3_column_int(stmt, 0);
        int lessonNumber = sqlite3_column_int(stmt, 1);
        const char* group= (const char*)sqlite3_column_text(stmt, 2);
        const char* subj = (const char*)sqlite3_column_text(stmt, 3);
        const char* room = (const char*)sqlite3_column_text(stmt, 4);
        int subgroup     = sqlite3_column_int(stmt, 5);
        const char* ltype= (const char*)sqlite3_column_text(stmt, 6);

        if (weekday != currentDay) {
            currentDay = weekday;

            std::string dateISO;
            std::string dateLabel;
            if (db_->getDateForWeekday(week_of_cycle, weekday, dateISO)) {
                // YYYY-MM-DD -> DD-MM
                if (dateISO.size() == 10) {
                    dateLabel = dateISO.substr(8, 2) + "-" + dateISO.substr(5, 2);
                }
            }

            if (!dateLabel.empty()) {
                std::cout << "\n[" << dayName[weekday] << "] (" << dateLabel << ")\n";
            } else {
                std::cout << "\n[" << dayName[weekday] << "]\n";
            }
        }


        std::string subjectText = subj ? subj : "";
        if (subgroup == 1)
            subjectText += " (подгр. 1)";
        else if (subgroup == 2)
            subjectText += " (подгр. 2)";

        std::string typeLabel;
        if (ltype && *ltype) {
            typeLabel = " [" + std::string(ltype) + "]";
        }

        std::cout << "  " << lessonNumber << "-я пара";
        if (lessonNumber >= 1 && lessonNumber <= 6) {
            std::cout << " (" << pairTimes[lessonNumber - 1] << ")";
        }

        std::cout << " | "
                  << (group ? group : "")
                  << " | " << subjectText << typeLabel
                  << " | " << (room ? room : "") << "\n";
    }

    if (!found) {
        std::cout << "Расписание не найдено.\n";
    }

    std::cout << "╚════════════════════════════════════════════════╝\n";

    sqlite3_finalize(stmt);
    return true;
}






bool Admin::listAllGroups() {
    if (!db_) return false;

    std::vector<std::pair<int, std::string>> groups;
    if (!db_->getAllGroups(groups) || groups.empty()) {
        std::cout << "Нет групп.\n";
        return false;
    }

    std::cout << "\n[Список групп]:\n";
    for (const auto& g : groups) {
        std::cout << g.first << " → " << g.second << "\n";
    }
    return true;
}

// ===== СУЩЕСТВУЮЩЕЕ МЕНЮ АДМИНА (ТВОЙ КОД, ОПТИЧИСТЕННЫЙ) =====

void Admin::displayMenu(Database& db) {
    while (true) {
        std::cout << "\n[ADMIN MENU]\n";
        std::cout << "1. Показать всех пользователей\n";
        std::cout << "2. Показать группы\n";
        std::cout << "3. Показать стипендию студента за сессию\n";
        std::cout << "4. Создать нового пользователя\n";
        std::cout << "5. Удалить пользователя по ID\n";
        std::cout << "6. Редактировать расписание (добавить пару)\n";
        std::cout << "7. Показать расписание группы\n";
        std::cout << "8. Показать расписание преподавателя\n";
        std::cout << "0. Выход в авторизацию\n";

        int choice = 0;
        std::cin >> choice;

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            std::vector<std::tuple<int, std::string, std::string, std::string, int, int>> users;
            if (!db.getAllUsers(users) || users.empty()) {
                std::cout << "Пользователи не найдены.\n";
                break;
            }

            std::cout << "id | username | name | role | groupid | subgroup\n";
            for (const auto& u : users) {
                int id, groupId, subgroup;
                std::string username, name, role;
                std::tie(id, username, name, role, groupId, subgroup) = u;
                std::cout << id << " | " << username << " | " << name
                          << " | " << role << " | " << groupId
                          << " | " << subgroup << "\n";
            }
            break;
        }

        case 2: {
            std::vector<std::pair<int, std::string>> groups;
            if (!db.getAllGroups(groups) || groups.empty()) {
                std::cout << "Нет групп.\n";
                break;
            }

            std::cout << "\nГруппы:\n";
            for (const auto& g : groups) {
                std::cout << g.first << ". " << g.second << "\n";
            }
            break;
        }

        case 3: {
            int studentId = 0;
            std::cout << "Введите ID студента: ";
            std::cin >> studentId;

            std::vector<std::pair<int, std::string>> semesters;
            if (!db.getAllSemesters(semesters) || semesters.empty()) {
                std::cout << "Нет сессий/семестров.\n";
                break;
            }

            std::cout << "\nДоступные сессии:\n";
            for (const auto& s : semesters) {
                std::cout << s.first << ". " << s.second << "\n";
            }

            int semesterId = 0;
            std::cout << "Введите id сессии: ";
            std::cin >> semesterId;

            double avg = Statistics::calculateStudentAverage(db, studentId, semesterId);
            ScholarshipInfo info = ScholarshipCalculator::calculate(avg);

            int totalAbsencesHours = 0;
            db.getStudentTotalAbsences(studentId, semesterId, totalAbsencesHours);

            int unexcusedHours = 0;
            if (!db.getStudentUnexcusedAbsences(studentId, semesterId, unexcusedHours)) {
                std::cout << "[!] Не удалось получить данные о неуважительных пропусках.\n";
            }

            bool strippedByAbsences = false;
            if (unexcusedHours >= 30) {
                strippedByAbsences = true;
                info.coefficient = 0.0;
                info.amountBYN = 0.0;
            }

            std::cout << "Суммарные пропуски за семестр: " << totalAbsencesHours << " часов\n";
            std::cout << "Из них неуважительные: " << unexcusedHours << " часов\n";

            if (info.coefficient <= 0.0) {
                if (strippedByAbsences) {
                    std::cout << "Стипендия не выплачивается (пропуски 30 часов и более за семестр).\n";
                } else {
                    std::cout << "Стипендия не выплачивается (средний балл ниже 5.0).\n";
                }
            } else {
                std::cout << "Коэффициент: " << info.coefficient << "\n";
                std::cout << "Размер стипендии: " << info.amountBYN << " BYN\n";
            }
            break;
        }

        case 4: {
            std::string username, password, role, name;
            int groupId = 0;
            int subgroup = 0;

            std::cout << "Введите логин (username): ";
            std::cin >> username;
            std::cout << "Введите пароль: ";
            std::cin >> password;
            std::cout << "Введите роль (admin/teacher/student): ";
            std::cin >> role;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Введите ФИО: ";
            std::getline(std::cin, name);

            if (role == "student") {
                std::vector<std::pair<int, std::string>> groups;
                if (!db.getAllGroups(groups) || groups.empty()) {
                    std::cout << "Нет групп. Нельзя создать студента.\n";
                    break;
                }

                std::cout << "\nГруппы:\n";
                for (const auto& g : groups) {
                    std::cout << g.first << ". " << g.second << "\n";
                }

                std::cout << "Введите id группы: ";
                std::cin >> groupId;

                std::string surname = name;
                auto pos = surname.find(' ');
                if (pos != std::string::npos) {
                    surname = surname.substr(0, pos);
                }

                if (!surname.empty() && surname < "М") {
                    subgroup = 1;
                } else {
                    subgroup = 2;
                }
            } else {
                groupId = 0;
                subgroup = 0;
            }

            if (role != "admin" && role != "teacher" && role != "student") {
                std::cout << "Некорректная роль.\n";
                break;
            }

            if (db.insertUser(username, password, role, name, groupId, subgroup)) {
                std::cout << "Пользователь успешно создан.\n";
            } else {
                std::cout << "Ошибка при создании пользователя (возможно, логин уже существует).\n";
            }
            break;
        }

        case 5: {
            int userId = 0;
            std::cout << "Введите ID пользователя для удаления: ";
            std::cin >> userId;

            if (userId <= 0) {
                std::cout << "Некорректный ID.\n";
                break;
            }

            if (userId == getId()) {
                std::cout << "Нельзя удалить самого себя.\n";
                break;
            }

            if (db.deleteUserById(userId)) {
                std::cout << "Пользователь удалён.\n";
            } else {
                std::cout << "Не удалось удалить пользователя (возможно, ID не существует).\n";
            }
            break;
        }

case 6: {
    std::cout << "\n=== Редактирование расписания ===\n";

    // --- выбор группы ---
    int groupId = 0;
    std::cout << "ID группы (1-4): ";
    std::cin >> groupId;
    if (groupId < 1 || groupId > 4) {
        std::cout << "Некорректный id группы.\n";
        break;
    }

    int week = chooseWeekOfCycleOrDate(db);
    if (week == 0) {
        break;
    }

    // цикл редактирования одной недели/группы
    while (true) {
        viewScheduleForGroup(groupId, 0, week);

        std::cout << "\n1. Добавить пару\n";
        std::cout << "2. Изменить пару\n";
        std::cout << "3. Удалить пару\n";
        std::cout << "0. Вернуться в меню\n";
        std::cout << "Ваш выбор: ";
        int action = 0;
        std::cin >> action;

        if (action == 0) break;

        // ===== ДОБАВИТЬ ПАРУ =====
        if (action == 1) {
            int subgroup = 0;
            std::cout << "Подгруппа (0 - общая, 1/2): ";
            std::cin >> subgroup;
            if (subgroup != 0 && subgroup != 1 && subgroup != 2) {
                std::cout << "Некорректная подгруппа.\n";
                continue;
            }

            int weekday = 0;
            std::cout << "День недели (0-Пн..5-Сб): ";
            std::cin >> weekday;
            if (weekday < 0 || weekday > 5) {
                std::cout << "Некорректный день недели.\n";
                continue;
            }

            int lessonNumber = 0;
            std::cout << "Номер пары (1-6): ";
            std::cin >> lessonNumber;
            if (lessonNumber < 1 || lessonNumber > 6) {
                std::cout << "Некорректный номер пары.\n";
                continue;
            }

            // слот по группе
            if (db.isScheduleSlotBusy(groupId, subgroup, weekday, lessonNumber, week)) {
                std::cout << "В этом слоте у группы уже есть пара.\n";
                continue;
            }

            // предмет
            std::vector<std::pair<int, std::string>> subjects;
            if (!db.getAllSubjects(subjects) || subjects.empty()) {
                std::cout << "Нет предметов.\n";
                continue;
            }
            std::cout << "\nПредметы:\n";
            for (const auto& s : subjects) {
                std::cout << s.first << ". " << s.second << "\n";
            }
            int subjectId = 0;
            std::cout << "ID предмета: ";
            std::cin >> subjectId;

            // преподаватель + его предметы
            std::vector<std::tuple<int, std::string, std::string>> teachers;
            if (!db.getTeachersWithSubjects(teachers) || teachers.empty()) {
                std::cout << "Нет преподавателей.\n";
                continue;
            }

            // сортируем по ID
            std::sort(teachers.begin(), teachers.end(),
                      [](const auto& a, const auto& b) {
                          return std::get<0>(a) < std::get<0>(b);
                      });

            std::cout << "\nПреподаватели:\n";
            for (const auto& t : teachers) {
                std::cout << std::get<0>(t) << ". "
                          << std::get<1>(t) << " ("
                          << std::get<2>(t) << ")\n";
            }
            int teacherId = 0;
            std::cout << "ID преподавателя: ";
            std::cin >> teacherId;

            // занятость преподавателя
            if (db.isTeacherBusy(teacherId, weekday, lessonNumber, week)) {
                std::cout << "Преподаватель уже занят в это время.\n";
                continue;
            }

            // аудитория: корпус + номер
            int building = 0;
            std::cout << "Корпус (1-5, 0 - спортзал): ";
            std::cin >> building;

            std::string room;
            if (building == 0) {
                room = "спортзал";
            } else if (building >= 1 && building <= 5) {
                std::string roomNum;
                std::cout << "Номер аудитории: ";
                std::cin >> roomNum;
                room = roomNum + "-" + std::to_string(building);
            } else {
                std::cout << "Некорректный корпус.\n";
                continue;
            }

            // занятость аудитории
            if (db.isRoomBusy(room, weekday, lessonNumber, week)) {
                std::cout << "Аудитория уже занята.\n";
                continue;
            }

            // тип пары
            std::cout << "\nТип пары:\n";
            std::cout << "1. ЛК (лекция)\n";
            std::cout << "2. ПЗ (практика)\n";
            std::cout << "3. ЛР (лабораторная)\n";
            std::cout << "4. Пусто\n";
            std::cout << "Выбор: ";
            int typeChoice = 0;
            std::cin >> typeChoice;

            std::string lessonType;
            switch (typeChoice) {
                case 1: lessonType = "ЛК"; break;
                case 2: lessonType = "ПЗ"; break;
                case 3: lessonType = "ЛР"; break;
                case 4: lessonType = "";  break;
                default:
                    std::cout << "Некорректный выбор типа.\n";
                    continue;
            }

            // спросить, повторять ли пару на все недели цикла
            std::cout << "Повторять эту пару на все недели цикла (1-4)? (1 - да, 0 - нет): ";
            int repeatAll = 0;
            std::cin >> repeatAll;

            bool ok = true;
            if (repeatAll == 1) {
                for (int w = 1; w <= 4; ++w) {
                    if (!db.addScheduleEntry(groupId, subgroup, weekday, lessonNumber,
                                             w, subjectId, teacherId, room, lessonType)) {
                        ok = false;
                        break;
                    }
                }
            } else {
                ok = db.addScheduleEntry(groupId, subgroup, weekday, lessonNumber,
                                         week, subjectId, teacherId, room, lessonType);
            }

            if (ok) {
                std::cout << "Пара добавлена.\n";
            } else {
                std::cout << "Ошибка при добавлении пары.\n";
            }
        }

        // ===== ИЗМЕНИТЬ ПАРУ =====
        else if (action == 2) {
            int scheduleId = 0;
            std::cout << "ID пары для изменения: ";
            std::cin >> scheduleId;

            sqlite3* raw_db = db.getHandle();
            const char* sql = R"(
                SELECT weekday, lessonnumber, subgroup,
                       subjectid, teacherid, room, lesson_type, groupid
                FROM schedule
                WHERE id = ?
            )";

            sqlite3_stmt* stmt = nullptr;
            int rc2 = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
            if (rc2 != SQLITE_OK) {
                std::cout << "Ошибка поиска пары.\n";
                continue;
            }
            sqlite3_bind_int(stmt, 1, scheduleId);
            rc2 = sqlite3_step(stmt);
            if (rc2 != SQLITE_ROW) {
                std::cout << "Пара с таким ID не найдена.\n";
                sqlite3_finalize(stmt);
                continue;
            }

            int curWeekday   = sqlite3_column_int(stmt, 0);
            int curLesson    = sqlite3_column_int(stmt, 1);
            int curSubgroup  = sqlite3_column_int(stmt, 2);
            int curSubjectId = sqlite3_column_int(stmt, 3);
            int curTeacherId = sqlite3_column_int(stmt, 4);
            const char* r    = (const char*)sqlite3_column_text(stmt, 5);
            const char* t    = (const char*)sqlite3_column_text(stmt, 6);
            int curGroupId   = sqlite3_column_int(stmt, 7);

            std::string curRoom = r ? r : "";
            std::string curType = t ? t : "";
            sqlite3_finalize(stmt);

            int newWeekday  = curWeekday;
            int newLesson   = curLesson;
            int newSubgroup = curSubgroup;
            int newSubject  = curSubjectId;
            int newTeacher  = curTeacherId;
            std::string newRoom = curRoom;
            std::string newType = curType;
            int newGroupId = curGroupId; // для лекций может быть 0

            int tmp = 0;

            std::cout << "Новый день недели (0-5, -1 не менять): ";
            std::cin >> tmp;
            if (tmp != -1) {
                if (tmp < 0 || tmp > 5) {
                    std::cout << "Некорректный день.\n";
                    continue;
                }
                newWeekday = tmp;
            }

            std::cout << "Новый номер пары (1-6, -1 не менять): ";
            std::cin >> tmp;
            if (tmp != -1) {
                if (tmp < 1 || tmp > 6) {
                    std::cout << "Некорректный номер пары.\n";
                    continue;
                }
                newLesson = tmp;
            }

            std::cout << "Новая подгруппа (0/1/2, -1 не менять): ";
            std::cin >> tmp;
            if (tmp != -1) {
                if (tmp != 0 && tmp != 1 && tmp != 2) {
                    std::cout << "Некорректная подгруппа.\n";
                    continue;
                }
                newSubgroup = tmp;
            }

            // проверка слота (только если это не лекция)
            if (curType != "ЛК" &&
                (newWeekday != curWeekday || newLesson != curLesson || newSubgroup != curSubgroup)) {
                if (db.isScheduleSlotBusy(groupId, newSubgroup, newWeekday, newLesson, week)) {
                    std::cout << "На новый слот уже назначена пара.\n";
                    continue;
                }
            }

            std::cout << "Менять предмет? (1 - да, 0 - нет): ";
            std::cin >> tmp;
            if (tmp == 1) {
                std::vector<std::pair<int, std::string>> subjects;
                if (!db.getAllSubjects(subjects) || subjects.empty()) {
                    std::cout << "Нет предметов.\n";
                    continue;
                }
                std::cout << "Предметы:\n";
                for (const auto& s : subjects) {
                    std::cout << s.first << ". " << s.second << "\n";
                }
                std::cout << "ID предмета: ";
                std::cin >> newSubject;
            }

            std::cout << "Менять преподавателя? (1 - да, 0 - нет): ";
            std::cin >> tmp;
            if (tmp == 1) {
                std::vector<std::tuple<int, std::string, std::string>> teachers;
                if (!db.getTeachersWithSubjects(teachers) || teachers.empty()) {
                    std::cout << "Нет преподавателей.\n";
                    continue;
                }
                std::cout << "Преподаватели:\n";
                for (const auto& t3 : teachers) {
                    std::cout << std::get<0>(t3) << ". "
                              << std::get<1>(t3) << " ("
                              << std::get<2>(t3) << ")\n";
                }
                std::cout << "ID преподавателя: ";
                std::cin >> newTeacher;

                if (newTeacher != curTeacherId &&
                    db.isTeacherBusy(newTeacher, newWeekday, newLesson, week)) {
                    std::cout << "Новый преподаватель уже занят в это время.\n";
                    continue;
                }
            }

            std::cout << "Менять аудиторию? (1 - да, 0 - нет): ";
            std::cin >> tmp;
            if (tmp == 1) {
                int building = 0;
                std::cout << "Корпус (1-5, 0 - спортзал): ";
                std::cin >> building;

                if (building == 0) {
                    newRoom = "спортзал";
                } else if (building >= 1 && building <= 5) {
                    std::string roomNum;
                    std::cout << "Номер аудитории: ";
                    std::cin >> roomNum;
                    newRoom = roomNum + "-" + std::to_string(building);
                } else {
                    std::cout << "Некорректный корпус.\n";
                    continue;
                }

                if (newRoom != curRoom &&
                    db.isRoomBusy(newRoom, newWeekday, newLesson, week)) {
                    std::cout << "Новая аудитория уже занята.\n";
                    continue;
                }
            }

            std::cout << "Менять тип пары? (1 - да, 0 - нет): ";
            std::cin >> tmp;
            if (tmp == 1) {
                std::cout << "Тип пары:\n1. ЛК\n2. ПЗ\n3. ЛР\n4. Пусто\nВыбор: ";
                int typeChoice = 0;
                std::cin >> typeChoice;
                switch (typeChoice) {
                    case 1: newType = "ЛК"; break;
                    case 2: newType = "ПЗ"; break;
                    case 3: newType = "ЛР"; break;
                    case 4: newType = "";  break;
                    default:
                        std::cout << "Некорректный выбор типа.\n";
                        continue;
                }
            }

            if (db.updateScheduleEntry(scheduleId,
                                       newGroupId,
                                       newSubgroup,
                                       newWeekday, newLesson, week,
                                       newSubject, newTeacher,
                                       newRoom, newType)) {
                std::cout << "Пара изменена.\n";
            } else {
                std::cout << "Ошибка при изменении пары.\n";
            }
        }

        // ===== УДАЛИТЬ ПАРУ =====
        else if (action == 3) {
            int scheduleId = 0;
            std::cout << "ID пары для удаления: ";
            std::cin >> scheduleId;

            std::cout << "Точно удалить? (1 - да, 0 - нет): ";
            int confirm = 0;
            std::cin >> confirm;
            if (confirm != 1) {
                std::cout << "Отмена удаления.\n";
                continue;
            }

            if (db.deleteScheduleEntry(scheduleId)) {
                std::cout << "Пара удалена.\n";
            } else {
                std::cout << "Ошибка при удалении пары.\n";
            }
        }

        else {
            std::cout << "Некорректный выбор.\n";
        }
    }

    break;
}




            case 7: {
            int groupId = 0;
            std::cout << "\nID группы (1-4):";
            std::cin >> groupId;
            if (groupId <= 0) {
                std::cout << "Некорректный id группы.\n";
                break;
            }

            int subgroup = 0;
            std::cout << "\nПодгруппа (0 – вся группа, 1 или 2):";
            std::cin >> subgroup;

            int week = chooseWeekOfCycleOrDate(db);
            if (week == 0) {
                break;
            }

            viewScheduleForGroup(groupId, subgroup, week);
            break;
            }






            case 8: {
            // показать всех преподавателей, отсортированных по ID
            std::vector<std::pair<int,std::string>> teachers;
            if (!db.getAllTeachers(teachers) || teachers.empty()) {
                std::cout << "Нет преподавателей.\n";
                break;
            }

            // на всякий случай сортируем по id
            std::sort(teachers.begin(), teachers.end(),
                      [](const auto& a, const auto& b){ return a.first < b.first; });

            std::cout << "\nПреподаватели:\n";
            for (const auto& t : teachers) {
                std::cout << t.first << ". " << t.second << "\n";
            }

            int teacherId = 0;
            std::cout << "ID преподавателя: ";
            std::cin >> teacherId;

            int week = chooseWeekOfCycleOrDate(db);
            if (week == 0) break;

            viewScheduleForTeacher(teacherId, week);
            break;
            }







        default:
            std::cout << "Неверный пункт меню.\n";
            break;
        }
    }
}
