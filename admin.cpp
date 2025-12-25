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
Admin::Admin(int adminId,
             const std::string& username,
             const std::string& name,
             Database* db)
    : User(adminId, username, name, "admin"),
      adminId(adminId),
      db(db) {}


// ===== НОВЫЕ ФУНКЦИИ РАСПИСАНИЯ =====

bool Admin::viewScheduleForGroup(int groupId, int subgroup, int weekOfCycle) {
    if (!db || !db->isConnected()) {
        std::cerr << "[✗] БД не инициализирована\n";
        return false;
    }

    sqlite3* raw_db = db->getHandle();
    const char* sql = R"(
        SELECT
            sch.id,            -- 0
            sch.weekday,       -- 1
            sch.lessonnumber,  -- 2
            subj.name,         -- 3
            u.name,            -- 4
            sch.room,          -- 5
            sch.subgroup,      -- 6
            sch.lessontype     -- 7
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        JOIN users    u    ON sch.teacherid = u.id
        WHERE (sch.groupid = ? OR sch.groupid = 0)
          AND sch.weekofcycle = ?
        ORDER BY sch.weekday, sch.lessonnumber, sch.subgroup
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] prepare error: " << sqlite3_errmsg(raw_db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, weekOfCycle);

    const auto& dayNames  = getDayNames();
    const auto& pairTimes = getPairTimes();

    std::cout << "\n╔════════════════════════════════════════╗\n"
              << "║ РАСПИСАНИЕ ГРУППЫ " << groupId
              << " (неделя " << weekOfCycle << ")"
              << " ║\n"
              << "╚════════════════════════════════════════╝\n";

    bool found = false;
    int currentDay = -1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId   = sqlite3_column_int(stmt, 0);
        int weekday      = sqlite3_column_int(stmt, 1);
        int lessonNumber = sqlite3_column_int(stmt, 2);
        const char* subj = (const char*)sqlite3_column_text(stmt, 3);
        const char* teach= (const char*)sqlite3_column_text(stmt, 4);
        const char* room = (const char*)sqlite3_column_text(stmt, 5);
        int rowSubGroup  = sqlite3_column_int(stmt, 6);
        const char* ltype= (const char*)sqlite3_column_text(stmt, 7);

        if (weekday < 0 || weekday > 5) continue;

        // фильтр подгруппы: 0 -> все, иначе показываем 0 и выбранную
        if (subgroup != 0 && rowSubGroup != 0 && rowSubGroup != subgroup) continue;

        found = true;

        if (weekday != currentDay) {
            currentDay = weekday;

            std::string dateISO;
            std::string dateLabel;
            if (db->getDateForWeekday(weekOfCycle, weekday, dateISO)) {
                dateLabel = formatDateLabel(dateISO);
            }

            if (!dateLabel.empty())
                std::cout << "\n[" << dayNames[weekday] << "] (" << dateLabel << ")\n";
            else
                std::cout << "\n[" << dayNames[weekday] << "]\n";
        }

        std::string subjectText = subj ? subj : "";
        if (rowSubGroup == 1) subjectText += " (подгр. 1)";
        else if (rowSubGroup == 2) subjectText += " (подгр. 2)";

        std::string typeLabel;
        if (ltype && *ltype) typeLabel = " [" + std::string(ltype) + "]";

        std::cout << " ID " << scheduleId << " | Пара " << lessonNumber;
        if (lessonNumber >= 1 && lessonNumber <= 6) {
            std::cout << " (" << pairTimes[lessonNumber - 1] << ")";
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


bool Admin::viewScheduleForTeacher(int teacherId, int weekOfCycle) {
    if (!db || !db->isConnected()) {
        std::cerr << "[✗] Нет соединения с БД.\n";
        return false;
    }

    sqlite3* raw_db = db->getHandle();
    const char* sql = R"(
        SELECT
            sch.weekday,        -- 0
            sch.lessonnumber,   -- 1
            g.name AS groupname, -- 2
            subj.name AS subject, -- 3
            sch.room,           -- 4
            sch.subgroup,       -- 5
            sch.lessontype      -- 6
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        JOIN groups   g    ON sch.groupid   = g.id
        WHERE sch.teacherid    = ?
          AND sch.weekofcycle  = ?
        ORDER BY sch.weekday, sch.lessonnumber, sch.subgroup
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] prepare error: " << sqlite3_errmsg(raw_db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, weekOfCycle);

    const auto& dayNames  = getDayNames();
    const auto& pairTimes = getPairTimes();

    std::cout << "\n╔═══ РАСПИСАНИЕ ПРЕПОДАВАТЕЛЯ ID "
              << teacherId << " (неделя " << weekOfCycle << ") ═══╗\n";

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

        if (weekday < 0 || weekday > 5) continue;

        if (weekday != currentDay) {
            currentDay = weekday;

            std::string dateISO;
            std::string dateLabel;
            if (db->getDateForWeekday(weekOfCycle, weekday, dateISO)) {
                dateLabel = formatDateLabel(dateISO);
            }

            if (!dateLabel.empty())
                std::cout << "\n[" << dayNames[weekday] << "] (" << dateLabel << ")\n";
            else
                std::cout << "\n[" << dayNames[weekday] << "]\n";
        }

        std::string subjectText = subj ? subj : "";
        if (subgroup == 1) subjectText += " (подгр. 1)";
        else if (subgroup == 2) subjectText += " (подгр. 2)";

        std::string typeLabel;
        if (ltype && *ltype) typeLabel = " [" + std::string(ltype) + "]";

        std::cout << "  " << lessonNumber << "-я пара";
        if (lessonNumber >= 1 && lessonNumber <= 6) {
            std::cout << " (" << pairTimes[lessonNumber - 1] << ")";
        }

        std::cout << " | " << (group ? group : "")
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
    if (!db) return false;

    std::vector<std::pair<int, std::string>> groups;
    if (!db->getAllGroups(groups) || groups.empty()) {
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
        std::cout << "6. Редактировать расписание (добавить/изменить/удалить пару)\n";
        std::cout << "7. Показать расписание группы\n";
        std::cout << "8. Показать расписание преподавателя\n";
        std::cout << "0. Выход в авторизацию\n";

        int choice = readChoiceFromList("Ваш выбор", 0, 8, false, 0);
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
            int studentId = readIntInRange("Введите ID студента", 1, 1000000, 1, true, 0);
            if (studentId == 0) break;

            std::vector<std::pair<int, std::string>> semesters;
            if (!db.getAllSemesters(semesters) || semesters.empty()) {
                std::cout << "Нет сессий/семестров.\n";
                break;
            }

            std::cout << "\nДоступные сессии:\n";
            int minSem = semesters.front().first, maxSem = semesters.front().first;
            for (const auto& s : semesters) {
                std::cout << s.first << ". " << s.second << "\n";
                minSem = std::min(minSem, s.first);
                maxSem = std::max(maxSem, s.first);
            }

            int semesterId = readIntInRange("Введите id сессии", minSem, maxSem, minSem, true, 0);
            if (semesterId == 0) break;

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
            std::string username = readToken("Введите логин (username)");
            std::string password = readToken("Введите пароль");
            std::string role     = readToken("Введите роль (admin/teacher/student)");

            std::string name;
            std::cout << "Введите ФИО: ";
            std::getline(std::cin >> std::ws, name);

            int groupId = 0;
            int subgroup = 0;

            if (role == "student") {
                std::vector<std::pair<int, std::string>> groups;
                if (!db.getAllGroups(groups) || groups.empty()) {
                    std::cout << "Нет групп. Нельзя создать студента.\n";
                    break;
                }

                std::cout << "\nГруппы:\n";
                int minG = groups.front().first, maxG = groups.front().first;
                for (const auto& g : groups) {
                    std::cout << g.first << ". " << g.second << "\n";
                    minG = std::min(minG, g.first);
                    maxG = std::max(maxG, g.first);
                }

                groupId = readIntInRange("Введите id группы", minG, maxG, minG, true, 0);
                if (groupId == 0) break;

                std::string surname = name;
                auto pos = surname.find(' ');
                if (pos != std::string::npos) surname = surname.substr(0, pos);

                subgroup = (!surname.empty() && surname < "М") ? 1 : 2;
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
            int userId = readIntInRange("Введите ID пользователя для удаления", 1, 1000000, 1, true, 0);
            if (userId == 0) break;

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

            int groupId = readIntInRange("Введите ID группы (1-4)", 1, 4, 1, true, 0);
            if (groupId == 0) break;

            int week = chooseWeekOfCycleOrDate(db);
            if (week == 0) break;

            while (true) {
                viewScheduleForGroup(groupId, 0, week);

                std::cout << "\n1. Добавить пару\n";
                std::cout << "2. Изменить пару\n";
                std::cout << "3. Удалить пару\n";
                std::cout << "0. Вернуться в меню\n";

                int action = readChoiceFromList("Действие", 0, 3, false, 0);
                if (action == 0) break;

                // ===== ДОБАВИТЬ ПАРУ =====
                if (action == 1) {
                    int subgroup = readIntInRange("Подгруппа (0-общая/1/2)", 0, 2, 0, true, -1);
                    if (subgroup == -1) continue;

                    int weekday = readIntInRange("День недели (0..5)", 0, 5, 0, true, -1);
                    if (weekday == -1) continue;

                    int lessonNumber = readIntInRange("Номер пары (1..6)", 1, 6, 1, true, -1);
                    if (lessonNumber == -1) continue;

                    if (db.isScheduleSlotBusy(groupId, subgroup, weekday, lessonNumber, week)) {
                        std::cout << "В этом слоте у группы уже есть пара.\n";
                        continue;
                    }

                    std::vector<std::pair<int, std::string>> subjects;
                    if (!db.getAllSubjects(subjects) || subjects.empty()) {
                        std::cout << "Нет предметов.\n";
                        continue;
                    }

                    std::cout << "\nПредметы:\n";
                    int minSub = subjects.front().first, maxSub = subjects.front().first;
                    for (const auto& s : subjects) {
                        std::cout << s.first << ". " << s.second << "\n";
                        minSub = std::min(minSub, s.first);
                        maxSub = std::max(maxSub, s.first);
                    }

                    int subjectId = readIntInRange("ID предмета", minSub, maxSub, minSub, true, -1);
                    if (subjectId == -1) continue;

                    std::vector<std::tuple<int, std::string, std::string>> teachers;
                    if (!db.getTeachersWithSubjects(teachers) || teachers.empty()) {
                        std::cout << "Нет преподавателей.\n";
                        continue;
                    }

                    std::sort(teachers.begin(), teachers.end(),
                              [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

                    std::cout << "\nПреподаватели:\n";
                    int minT = std::get<0>(teachers.front()), maxT = std::get<0>(teachers.front());
                    for (const auto& t : teachers) {
                        std::cout << std::get<0>(t) << ". " << std::get<1>(t)
                                  << " (" << std::get<2>(t) << ")\n";
                        minT = std::min(minT, std::get<0>(t));
                        maxT = std::max(maxT, std::get<0>(t));
                    }

                    int teacherId = readIntInRange("ID преподавателя", minT, maxT, minT, true, -1);
                    if (teacherId == -1) continue;

                    if (db.isTeacherBusy(teacherId, weekday, lessonNumber, week)) {
                        std::cout << "Преподаватель уже занят в это время.\n";
                        continue;
                    }

                    int building = readIntInRange("Корпус (0-без аудитории, 1..5)", 0, 5, 0, true, -1);
                    if (building == -1) continue;

                    std::string room;
                    if (building == 0) {
                        room = "";
                    } else {
                        std::string roomNum = readToken("Номер аудитории (например 123)");
                        room = roomNum + "-" + std::to_string(building);
                    }

                    if (!room.empty() && db.isRoomBusy(room, weekday, lessonNumber, week)) {
                        std::cout << "Аудитория уже занята.\n";
                        continue;
                    }

                    std::cout << "\nТип пары:\n";
                    std::cout << "1. ЛК (лекция)\n";
                    std::cout << "2. ПЗ (практика)\n";
                    std::cout << "3. ЛР (лабораторная)\n";
                    std::cout << "4. Пусто\n";

                    int typeChoice = readIntInRange("Выбор", 1, 4, 4, true, -1);
                    if (typeChoice == -1) continue;

                    std::string lessonType;
                    switch (typeChoice) {
                        case 1: lessonType = "ЛК"; break;
                        case 2: lessonType = "ПЗ"; break;
                        case 3: lessonType = "ЛР"; break;
                        case 4: lessonType = "";  break;
                    }

                    int repeatAll = readIntInRange("Повторять на все недели цикла? (0/1)", 0, 1, 0, true, -1);
                    if (repeatAll == -1) continue;

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

                    std::cout << (ok ? "Пара добавлена.\n" : "Ошибка при добавлении пары.\n");
                }

                // ===== ИЗМЕНИТЬ ПАРУ =====
                else if (action == 2) {
                    int scheduleId = readIntInRange("ID пары для изменения", 1, 1000000000, 1, true, 0);
                    if (scheduleId == 0) continue;

                    sqlite3* raw_db = db.getHandle();
                    const char* sql = R"(
                        SELECT weekday, lessonnumber, subgroup,
                               subjectid, teacherid, room, lessontype, groupid
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
                    int newGroupId = curGroupId;

                    int tmp = readIntInRange("Новый день недели (0-5, 6=не менять)", 0, 6, 6, false, 6);
                    if (tmp != 6) newWeekday = tmp;

                    tmp = readIntInRange("Новый номер пары (1-6, 0=не менять)", 0, 6, 0, false, 0);
                    if (tmp != 0) newLesson = tmp;

                    tmp = readIntInRange("Новая подгруппа (0/1/2, 3=не менять)", 0, 3, 3, false, 3);
                    if (tmp != 3) newSubgroup = tmp;

                    if (curType != "ЛК" &&
                        (newWeekday != curWeekday || newLesson != curLesson || newSubgroup != curSubgroup)) {
                        if (db.isScheduleSlotBusy(groupId, newSubgroup, newWeekday, newLesson, week)) {
                            std::cout << "На новый слот уже назначена пара.\n";
                            continue;
                        }
                    }

                    tmp = readIntInRange("Менять предмет? (0/1)", 0, 1, 0, false, 0);
                    if (tmp == 1) {
                        std::vector<std::pair<int, std::string>> subjects;
                        if (!db.getAllSubjects(subjects) || subjects.empty()) {
                            std::cout << "Нет предметов.\n";
                            continue;
                        }
                        int minSub = subjects.front().first, maxSub = subjects.front().first;
                        std::cout << "Предметы:\n";
                        for (const auto& s : subjects) {
                            std::cout << s.first << ". " << s.second << "\n";
                            minSub = std::min(minSub, s.first);
                            maxSub = std::max(maxSub, s.first);
                        }
                        newSubject = readIntInRange("ID предмета", minSub, maxSub, minSub, true, 0);
                        if (newSubject == 0) continue;
                    }

                    tmp = readIntInRange("Менять преподавателя? (0/1)", 0, 1, 0, false, 0);
                    if (tmp == 1) {
                        std::vector<std::tuple<int, std::string, std::string>> teachers;
                        if (!db.getTeachersWithSubjects(teachers) || teachers.empty()) {
                            std::cout << "Нет преподавателей.\n";
                            continue;
                        }
                        std::sort(teachers.begin(), teachers.end(),
                                  [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

                        int minT = std::get<0>(teachers.front()), maxT = std::get<0>(teachers.front());
                        std::cout << "Преподаватели:\n";
                        for (const auto& t3 : teachers) {
                            std::cout << std::get<0>(t3) << ". " << std::get<1>(t3)
                                      << " (" << std::get<2>(t3) << ")\n";
                            minT = std::min(minT, std::get<0>(t3));
                            maxT = std::max(maxT, std::get<0>(t3));
                        }

                        newTeacher = readIntInRange("ID преподавателя", minT, maxT, minT, true, 0);
                        if (newTeacher == 0) continue;

                        if (newTeacher != curTeacherId &&
                            db.isTeacherBusy(newTeacher, newWeekday, newLesson, week)) {
                            std::cout << "Новый преподаватель уже занят в это время.\n";
                            continue;
                        }
                    }

                    tmp = readIntInRange("Менять аудиторию? (0/1)", 0, 1, 0, false, 0);
                    if (tmp == 1) {
                        int building = readIntInRange("Корпус (0-без аудитории, 1..5)", 0, 5, 0, true, -1);
                        if (building == -1) continue;

                        if (building == 0) {
                            newRoom = "";
                        } else {
                            std::string roomNum = readToken("Номер аудитории");
                            newRoom = roomNum + "-" + std::to_string(building);
                        }

                        if (!newRoom.empty() && newRoom != curRoom &&
                            db.isRoomBusy(newRoom, newWeekday, newLesson, week)) {
                            std::cout << "Новая аудитория уже занята.\n";
                            continue;
                        }
                    }

                    tmp = readIntInRange("Менять тип пары? (0/1)", 0, 1, 0, false, 0);
                    if (tmp == 1) {
                        std::cout << "Тип пары:\n1. ЛК\n2. ПЗ\n3. ЛР\n4. Пусто\n";
                        int typeChoice = readIntInRange("Выбор", 1, 4, 4, true, -1);
                        if (typeChoice == -1) continue;

                        switch (typeChoice) {
                            case 1: newType = "ЛК"; break;
                            case 2: newType = "ПЗ"; break;
                            case 3: newType = "ЛР"; break;
                            case 4: newType = "";  break;
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
                    int scheduleId = readIntInRange("ID пары для удаления", 1, 1000000000, 1, true, 0);
                    if (scheduleId == 0) continue;

                    int confirm = readIntInRange("Точно удалить? (0/1)", 0, 1, 0, false, 0);
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
            }

            break;
        }

        case 7: {
            int groupId = readIntInRange("ID группы (1-4)", 1, 4, 1, true, 0);
            if (groupId == 0) break;

            int subgroup = readIntInRange("Подгруппа (0 – вся группа, 1 или 2)", 0, 2, 0, true, 0);
            if (subgroup == 0 && false) {} // просто чтобы не ругалось на стиль; не трогай

            int week = chooseWeekOfCycleOrDate(db);
            if (week == 0) break;

            viewScheduleForGroup(groupId, subgroup, week);
            break;
        }

        case 8: {
            std::vector<std::pair<int,std::string>> teachers;
            if (!db.getAllTeachers(teachers) || teachers.empty()) {
                std::cout << "Нет преподавателей.\n";
                break;
            }

            std::sort(teachers.begin(), teachers.end(),
                      [](const auto& a, const auto& b){ return a.first < b.first; });

            std::cout << "\nПреподаватели:\n";
            int minT = teachers.front().first, maxT = teachers.front().first;
            for (const auto& t : teachers) {
                std::cout << t.first << ". " << t.second << "\n";
                minT = std::min(minT, t.first);
                maxT = std::max(maxT, t.first);
            }

            int teacherId = readIntInRange("ID преподавателя", minT, maxT, minT, true, 0);
            if (teacherId == 0) break;

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

