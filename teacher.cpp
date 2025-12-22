#include "teacher.h"
#include "database.h"
#include "statistics.h"
#include "menu.h"

#include <iostream>
#include <vector>
#include <utility>
#include <tuple>
#include <iomanip>
#include <algorithm>
// ===== КОНСТРУКТОР =====

Teacher::Teacher(int teacher_id,
                 const std::string& username,
                 const std::string& name,
                 Database* db)
    : User(teacher_id, username, name, "teacher"),
      teacher_id_(teacher_id),
      db_(db) {}


// ===== СТРУКТУРА ДЛЯ РАСПИСАНИЯ ПАР =====
struct LessonInfo {
    int scheduleId;
    int subjectId;
    int dayOfWeek;
    int pairNumber;
    int subgroup;
    std::string subjectName;
};



// Выбор пары из расписания препода по конкретной группе.
// Заполняет outSubjectId и возвращает true при успехе.
static bool selectLessonFromSchedule(Database& db,
                                     int teacherId,
                                     int groupId,
                                     int& outSubjectId)
{
    std::vector<std::tuple<int,int,int,int,int,std::string>> rows;
    if (!db.getScheduleForTeacherGroup(teacherId, groupId, rows) || rows.empty()) {
        std::cout << "Для этой группы у вас нет пар по расписанию.\n";
        return false;
    }

    std::vector<LessonInfo> lessons;
    lessons.reserve(rows.size());
    for (const auto& r : rows) {
        LessonInfo les;
        std::tie(les.scheduleId,
                 les.subjectId,
                 les.dayOfWeek,
                 les.pairNumber,
                 les.subgroup,
                 les.subjectName) = r;
        lessons.push_back(les);
    }

    std::cout << "\nВаши пары для группы " << groupId << ":\n";
    for (size_t i = 0; i < lessons.size(); ++i) {
        const auto& les = lessons[i];
        std::cout << (i + 1) << ") День " << les.dayOfWeek
                  << ", пара " << les.pairNumber
                  << ", предмет: " << les.subjectName
                  << " (subjectId = " << les.subjectId << ")\n";
    }

    int choice = readChoiceFromList("Выберите номер пары", 1, (int)lessons.size(), true, 0);
    if (choice == 0) {
        std::cout << "Отмена.\n";
        return false;
    }

    outSubjectId = lessons[choice - 1].subjectId;
    return true;
}


// Выбор конкретной пары препода по группе и неделе с возвратом scheduleId и даты-слота.
static bool selectLessonForAbsence(Database& db,
                                   int teacherId,
                                   int groupId,
                                   int week_of_cycle,
                                   int& outScheduleId,
                                   int& outSubjectId,
                                   int& outWeekday,
                                   int& outLessonNumber)
{
    std::vector<std::tuple<int,int,int,int,int,std::string>> rows;
    if (!db.getScheduleForTeacherGroup(teacherId, groupId, rows) || rows.empty()) {
        std::cout << "Для этой группы у вас нет пар по расписанию.\n";
        return false;
    }

    std::vector<LessonInfo> lessons;
    lessons.reserve(rows.size());
    for (const auto& r : rows) {
        LessonInfo les;
        std::tie(les.scheduleId,
                 les.subjectId,
                 les.dayOfWeek,
                 les.pairNumber,
                 les.subgroup,
                 les.subjectName) = r;
        lessons.push_back(les);
    }

    const char* pairTimes[] = {
        "08:30-09:55",
        "10:05-11:30",
        "12:00-13:25",
        "13:35-15:00",
        "15:30-16:55",
        "17:05-18:30"
    };

    std::cout << "\nВаши пары для группы " << groupId
              << " (неделя " << week_of_cycle << "):\n";
    for (size_t i = 0; i < lessons.size(); ++i) {
        const auto& les = lessons[i];

        std::string dateISO;
        std::string dateLabel;
        if (db.getDateForWeekday(week_of_cycle, les.dayOfWeek, dateISO) && dateISO.size() == 10) {
            dateLabel = dateISO.substr(8, 2) + "-" + dateISO.substr(5, 2);
        }

        std::cout << (i + 1) << ") День " << les.dayOfWeek;
        if (!dateLabel.empty()) std::cout << " (" << dateLabel << ")";

        std::cout << ", " << les.pairNumber << "-я пара";
        if (les.pairNumber >= 1 && les.pairNumber <= 6)
            std::cout << " (" << pairTimes[les.pairNumber - 1] << ")";

        if (les.subgroup == 1) std::cout << ", подгр. 1";
        else if (les.subgroup == 2) std::cout << ", подгр. 2";

        std::cout << ", предмет: " << les.subjectName
                  << " (subjectId = " << les.subjectId << ")\n";
    }

    int choice = readChoiceFromList("Выберите номер пары", 1, (int)lessons.size(), true, 0);
    if (choice == 0) {
        std::cout << "Отмена.\n";
        return false;
    }

    const auto& sel = lessons[choice - 1];
    outScheduleId   = sel.scheduleId;
    outSubjectId    = sel.subjectId;
    outWeekday      = sel.dayOfWeek;
    outLessonNumber = sel.pairNumber;
    return true;
}



// ===== МЕНЮ ПРЕПОДАВАТЕЛЯ (ТВОЙ КОД) =====

void Teacher::displayMenu(Database& db) {
    while (true) {
        std::cout << "\n[TEACHER MENU]\n";
        std::cout << "1. Показать мои предметы (средний балл группы)\n";
        std::cout << "2. Показать журнал оценок\n";
        std::cout << "3. Поставить оценку студенту\n";
        std::cout << "4. Поставить пропуск студенту\n";
        std::cout << "5. Посмотреть пропуски студента\n";
        std::cout << "6. Удалить пропуск (только за указанную дату)\n";
        std::cout << "7. Сводка пропусков по группе и предмету\n";
        std::cout << "8. Посмотреть расписание на неделю\n";
        std::cout << "9. Посмотреть расписание на все 4 недели\n";
        std::cout << "10. Изменить или удалить оценку\n";
        std::cout << "0. Выход в авторизацию\n";

        int choice = readChoiceFromList("Ваш выбор", 0, 10, false, 0);
        if (choice == 0) break;


        switch (choice) {
            case 1: {
                std::vector<std::pair<int, std::string>> subjects;
                if (!db.getSubjectsForTeacher(getId(), subjects) || subjects.empty()) {
                    std::cout << "За вами не закреплены предметы.\n";
                    break;
                }

                std::cout << "\nВаши предметы:\n";
                for (const auto& s : subjects) {
                    std::cout << s.first << ". " << s.second << "\n";
                }

                int subjectId = readIntInRange("Введите id предмета", 1, 1000000, subjects.front().first, true, 0);
                if (subjectId == 0) break;

                std::vector<std::pair<int, std::string>> groups;
                if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                    std::cout << "За вами не закреплены группы.\n";
                    break;
                }

                std::cout << "\nДоступные группы:\n";
                for (const auto& g : groups) {
                    std::cout << g.first << ". " << g.second << "\n";
                }

                int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
                if (groupId == 0) break;

                int semesterId = 1;
                double avg = Statistics::calculateGroupSubjectAverage(db, groupId, subjectId, semesterId);
                std::cout << "\nСредний балл группы по предмету: " << avg << "\n";
                break;
            }
         case 2: {
            std::vector<std::pair<int, std::string>> subjects;
            if (!db.getSubjectsForTeacher(getId(), subjects) || subjects.empty()) {
                std::cout << "За вами не закреплены предметы.\n";
                break;
            }

            std::cout << "\nВаши предметы:\n";
            for (const auto& s : subjects) {
                std::cout << s.first << ". " << s.second << "\n";
            }

            int subjectId = readIntInRange("Введите id предмета", 1, 1000000, subjects.front().first, true, 0);
            if (subjectId == 0) break;

            std::vector<std::pair<int, std::string>> groups;
            if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                std::cout << "За вами не закреплены группы.\n";
                break;
            }

            std::cout << "\nГруппы:\n";
            for (const auto& g : groups) {
                std::cout << g.first << ". " << g.second << "\n";
            }

            int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
            if (groupId == 0) break;

            int semesterId = 1;

            std::vector<std::pair<int, std::string>> students;
            if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
                std::cout << "В группе нет студентов.\n";
                break;
            }

            std::cout << "\nЖурнал (группа " << groupId
                      << ", предмет " << subjectId
                      << ", сессия " << semesterId << ")\n";

            for (const auto& st : students) {
                int studentId = st.first;
                const std::string& name = st.second;

                std::vector<std::tuple<int, std::string, std::string>> grades;
                db.getStudentSubjectGrades(studentId, subjectId, semesterId, grades);

                std::cout << "\n" << name << " (ID " << studentId << "):\n";
                if (grades.empty()) {
                    std::cout << " нет оценок\n";
                    continue;
                }

                for (const auto& g : grades) {
                    int value;
                    std::string date, type;
                    std::tie(value, date, type) = g;
                    std::cout << " " << value << " | " << date << " | " << type << "\n";
                }
            }
            break;
        }
       case 3: {
            int semesterId = 1;

            std::vector<std::pair<int, std::string>> groups;
            if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                std::cout << "У вас нет привязанных групп.\n";
                break;
            }

            std::cout << "\nДоступные группы:\n";
            for (const auto& g : groups) {
                std::cout << " " << g.first << " - " << g.second << "\n";
            }

            int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
            if (groupId == 0) break;

            std::vector<std::pair<int, std::string>> students;
            if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
                std::cout << "В этой группе нет студентов.\n";
                break;
            }

            std::cout << "\nСтуденты группы:\n";
            for (const auto& st : students) {
                std::cout << " " << st.first << " - " << st.second << "\n";
            }

            int studentId = readIntInRange("Введите ID студента", 1, 1000000, students.front().first, true, 0);
            if (studentId == 0) break;

            int studentGroupId = 0, studentSubgroup = 0;
            if (!db.getStudentGroupAndSubgroup(studentId, studentGroupId, studentSubgroup)) {
                std::cout << "Ошибка: не удалось определить подгруппу студента.\n";
                break;
            }
            if (studentGroupId != groupId) {
                std::cout << "Ошибка: студент не из этой группы.\n";
                break;
            }

            int weekOfCycle = chooseWeekOfCycleOrDate(db);
            if (weekOfCycle == 0) break;

            sqlite3* rawdb = db.getHandle();
            const char* sql = R"(
                SELECT
sch.id,
                       sch.subject_id,
                       sch.weekday,
                       sch.lesson_number,
                       sch.sub_group,
                       subj.name,
                       sch.lesson_type
                FROM schedule sch
                JOIN subjects subj ON sch.subject_id = subj.id
                WHERE sch.teacher_id = ?
                  AND sch.group_id = ?
                  AND sch.week_of_cycle = ?
                  AND (sch.sub_group = 0 OR sch.sub_group = ?)
                  AND sch.lesson_type != 'ЛК'
                ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
            )";

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(rawdb, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                std::cout << "Ошибка SQL.\n";
                break;
            }

            sqlite3_bind_int(stmt, 1, getId());
            sqlite3_bind_int(stmt, 2, groupId);
            sqlite3_bind_int(stmt, 3, weekOfCycle);
            sqlite3_bind_int(stmt, 4, studentSubgroup);

            struct LessonInfo {
                int scheduleId;
                int subjectId;
                int weekday;
                int lessonNumber;
                int subgroup;
                std::string subjectName;
                std::string lessonType;
                std::string dateISO;
            };

            std::vector<LessonInfo> lessons;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                LessonInfo info;
                info.scheduleId   = sqlite3_column_int(stmt, 0);
                info.subjectId    = sqlite3_column_int(stmt, 1);
                info.weekday      = sqlite3_column_int(stmt, 2);
                info.lessonNumber = sqlite3_column_int(stmt, 3);
                info.subgroup     = sqlite3_column_int(stmt, 4);

                const char* subjName = (const char*)sqlite3_column_text(stmt, 5);
                const char* ltype    = (const char*)sqlite3_column_text(stmt, 6);
                info.subjectName = subjName ? subjName : "";
                info.lessonType  = ltype ? ltype : "";

                if (!db.getDateForWeekday(weekOfCycle, info.weekday, info.dateISO)) {
                    continue;
                }
                lessons.push_back(info);
            }
            sqlite3_finalize(stmt);

            if (lessons.empty()) {
                std::cout << "Нет доступных пар для оценки (только ПЗ/ЛР, подходящие по подгруппе).\n";
                break;
            }

            const char* pairTimes[] = {"08:30-09:55", "10:05-11:30", "12:00-13:25",
                                       "13:35-15:00", "15:30-16:55", "17:05-18:30"};
            const char* dayNames[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};

            std::cout << "\nВыберите пару для выставления оценки:\n";
            for (size_t i = 0; i < lessons.size(); ++i) {
                const auto& les = lessons[i];
                std::string dateLabel;
                if (les.dateISO.size() == 10)
                    dateLabel = les.dateISO.substr(8,2) + "-" + les.dateISO.substr(5,2);

                std::cout << (i + 1) << ") " << dayNames[les.weekday] << " " << dateLabel
                          << ", пара " << les.lessonNumber;
                if (les.lessonNumber >= 1 && les.lessonNumber <= 6)
                    std::cout << " (" << pairTimes[les.lessonNumber - 1] << ")";
                if (les.subgroup == 1) std::cout << ", подгр. 1";
                else if (les.subgroup == 2) std::cout << ", подгр. 2";
                std::cout << ", " << les.subjectName << " [" << les.lessonType << "]\n";
            }

            int choice = readChoiceFromList("Номер пары", 1, (int)lessons.size(), true, 0);
            if (choice == 0) {
                std::cout << "Отмена.\n";
                break;
            }

            const auto& selected = lessons[choice - 1];

            // Проверка: есть ли пропуск на эту дату и предмет?
            sqlite3_stmt* absStmt = nullptr;
            const char* absSQL =
                "SELECT COUNT(*) FROM absences "
                "WHERE student_id = ? AND subject_id = ? AND semester_id = ? AND date = ?";

            if (sqlite3_prepare_v2(rawdb, absSQL, -1, &absStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(absStmt, 1, studentId);
                sqlite3_bind_int(absStmt, 2, selected.subjectId);
                sqlite3_bind_int(absStmt, 3, semesterId);
                sqlite3_bind_text(absStmt, 4, selected.dateISO.c_str(), -1, SQLITE_TRANSIENT);

                if (sqlite3_step(absStmt) == SQLITE_ROW) {
                    int count = sqlite3_column_int(absStmt, 0);
                    if (count > 0) {
                        std::cout << "Ошибка: у студента есть пропуск на эту пару ("
                                  << selected.dateISO << "). Оценку ставить нельзя.\n";
                        sqlite3_finalize(absStmt);
                        break;
                    }
                }
                sqlite3_finalize(absStmt);
            }

            int value = readIntInRange("Введите оценку (0-10)", 0, 10, 0, true, -1);
            if (value == -1) {
                std::cout << "Отмена.\n";
                break;
            }

            if (db.addGrade(studentId, selected.subjectId, semesterId, value,
                            selected.dateISO, selected.lessonType))
            {
                std::cout << "✓ Оценка " << value << " добавлена за " << selected.subjectName
                          << " [" << selected.lessonType << "] на " << selected.dateISO << "\n";
            } else {
                std::cout << "Ошибка при добавлении оценки.\n";
            }

            break;
        }

        case 4: {
            addAbsence(db);
            break;
        }

        case 5: {
            viewStudentAbsences(db);
            break;
        }

       case 6: {
            int semesterId = 1;

            std::vector<std::pair<int, std::string>> groups;
            if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                std::cout << "У вас нет привязанных групп.\n";
                break;
            }

            std::cout << "\nВаши группы:\n";
            for (const auto& g : groups) {
                std::cout << " " << g.first << " - " << g.second << "\n";
            }

            int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
            if (groupId == 0) break;

            std::vector<std::pair<int, std::string>> students;
            if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
                std::cout << "В этой группе нет студентов.\n";
                break;
            }

            std::cout << "\nСтуденты группы:\n";
            for (const auto& st : students) {
                std::cout << " " << st.first << " - " << st.second << "\n";
            }

            int studentId = readIntInRange("Введите ID студента", 1, 1000000, students.front().first, true, 0);
            if (studentId == 0) break;

            std::vector<std::pair<int, std::string>> subjects;
            if (!db.getSubjectsForTeacher(getId(), subjects) || subjects.empty()) {
                std::cout << "У вас нет предметов.\n";
                break;
            }

            std::cout << "\nВаши предметы:\n";
            for (const auto& s : subjects) {
                std::cout << " " << s.first << " - " << s.second << "\n";
            }

            int subjectId = readIntInRange("Введите id предмета", 1, 1000000, subjects.front().first, true, 0);
            if (subjectId == 0) break;

            std::vector<std::tuple<std::string,int,std::string,std::string>> absences;
            if (!db.getStudentAbsencesForSemester(studentId, semesterId, absences)) {
                std::cout << "Ошибка при получении пропусков.\n";
                break;
            }
            if (absences.empty()) {
                std::cout << "У этого студента нет пропусков.\n";
                break;
            }

            std::cout << "\nВыберите пропуск для удаления:\n";
            for (size_t i = 0; i < absences.size(); ++i) {
                const auto& a = absences[i];
                const std::string& subjName = std::get<0>(a);
                int hours = std::get<1>(a);
                const std::string& date = std::get<2>(a);
                const std::string& type = std::get<3>(a);

                std::cout << (i + 1) << ") " << date << " | "
                          << subjName << " | " << hours << " часов | " << type << "\n";
            }

            int choice = readChoiceFromList("Номер пропуска", 1, (int)absences.size(), true, 0);
            if (choice == 0) {
                std::cout << "Отмена.\n";
                break;
            }

            const std::string dateISO = std::get<2>(absences[choice - 1]);
            if (db.deleteTodayAbsence(studentId, subjectId, semesterId, dateISO)) {
                std::cout << "Пропуск за " << dateISO << " удалён.\n";
            } else {
                std::cout << "Не удалось удалить пропуск.\n";
            }

            break;
        }


            case 7: {
            int semesterId = 1;

            std::vector<std::pair<int, std::string>> subjects;
            if (!db.getSubjectsForTeacher(getId(), subjects) || subjects.empty()) {
                std::cout << "За вами не закреплены предметы.\n";
                break;
            }

            std::cout << "\nВаши предметы:\n";
            for (const auto& s : subjects) {
                std::cout << s.first << ". " << s.second << "\n";
            }

            int subjectId = readIntInRange("Введите id предмета", 1, 1000000, subjects.front().first, true, 0);
            if (subjectId == 0) break;

            std::vector<std::pair<int, std::string>> groups;
            if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                std::cout << "За вами не закреплены группы.\n";
                break;
            }

            std::cout << "\nГруппы:\n";
            for (const auto& g : groups) {
                std::cout << g.first << ". " << g.second << "\n";
            }

            int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
            if (groupId == 0) break;

            std::vector<std::tuple<int,std::string,int>> rows;
            if (!db.getGroupSubjectAbsencesSummary(groupId, subjectId, semesterId, rows)) {
                std::cout << "✗ Ошибка при получении сводки пропусков.\n";
                break;
            }

            if (rows.empty()) {
                std::cout << "Нет студентов или пропусков по этой группе и предмету.\n";
                break;
            }

            std::cout << "\n=== Пропуски по группе " << groupId
                      << " и предмету " << subjectId << " ===\n";
            std::cout << "ID | ФИО | часы\n";
            std::cout << "─────────────────────────────\n";

            for (const auto& r : rows) {
                int studentId;
                std::string name;
                int totalHours;
                std::tie(studentId, name, totalHours) = r;
                std::cout << studentId << " | " << name << " | " << totalHours << "\n";
            }

            break;
        }

            case 8: {
                int week = chooseWeekOfCycleOrDate(db);
                if (week == 0) break;
                viewMySchedule(week);
                break;
            }




        case 9: {
            viewFullSchedule();
            break;
        }
case 10: {
            int semesterId = 1;

            std::vector<std::pair<int, std::string>> groups;
            if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
                std::cout << "У вас нет привязанных групп.\n";
                break;
            }

            std::cout << "\nВаши группы:\n";
            for (const auto& g : groups) {
                std::cout << " " << g.first << " - " << g.second << "\n";
            }

            int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
            if (groupId == 0) break;

            std::vector<std::pair<int, std::string>> students;
            if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
                std::cout << "В этой группе нет студентов.\n";
                break;
            }

            std::cout << "\nСтуденты:\n";
            for (const auto& st : students) {
                std::cout << " " << st.first << " - " << st.second << "\n";
            }

            int studentId = readIntInRange("Введите ID студента", 1, 1000000, students.front().first, true, 0);
            if (studentId == 0) break;

            std::vector<std::pair<int, std::string>> subjects;
            if (!db.getSubjectsForTeacher(getId(), subjects) || subjects.empty()) {
                std::cout << "У вас нет предметов.\n";
                break;
            }

            std::cout << "\nВаши предметы:\n";
            for (const auto& s : subjects) {
                std::cout << " " << s.first << " - " << s.second << "\n";
            }

            int subjectId = readIntInRange("Введите id предмета", 1, 1000000, subjects.front().first, true, 0);
            if (subjectId == 0) break;

            std::vector<std::tuple<int,int,std::string,std::string>> grades;
            if (!db.getGradesForStudentSubject(studentId, subjectId, semesterId, grades)) {
                std::cout << "Ошибка при получении оценок.\n";
                break;
            }

            if (grades.empty()) {
                std::cout << "Оценок нет.\n";
                break;
            }

            std::cout << "\nОценки студента:\n";
            for (size_t i = 0; i < grades.size(); ++i) {
                int gradeId, value;
                std::string date, type;
                std::tie(gradeId, value, date, type) = grades[i];
                std::cout << (i+1) << ") ID=" << gradeId << ", оценка=" << value
                          << ", дата=" << date << ", тип=" << type << "\n";
            }

            int choice = readChoiceFromList("Номер оценки", 1, (int)grades.size(), true, 0);
            if (choice == 0) {
                std::cout << "Отмена.\n";
                break;
            }

            int gradeId = std::get<0>(grades[choice - 1]);

            std::cout << "1 - Изменить, 2 - Удалить\n";
            int action = readIntInRange("Действие", 1, 2, 1, true, 0);
            if (action == 0) {
                std::cout << "Отмена.\n";
                break;
            }

            if (action == 1) {
                int newValue = readIntInRange("Новая оценка (0-10)", 0, 10, 0, true, -1);
                if (newValue == -1) {
                    std::cout << "Отмена.\n";
                    break;
                }

                std::string newDate = std::get<2>(grades[choice - 1]);
                std::string newType = std::get<3>(grades[choice - 1]);

                if (db.updateGrade(gradeId, newValue, newDate, newType)) {
                    std::cout << "✓ Оценка изменена.\n";
                } else {
                    std::cout << "Ошибка при изменении.\n";
                }
            } else {
                if (db.deleteGrade(gradeId)) {
                    std::cout << "✓ Оценка удалена.\n";
                } else {
                    std::cout << "Ошибка при удалении.\n";
                }
            }

            break;
        }

        default:
            std::cout << "Неверный пункт меню.\n";
            break;
        }
    }
}

// ===== ПРОПУСКИ (ТВОЙ КОД, ЧУТЬ ПОДЧИЩЕН) =====

void Teacher::addAbsence(Database& db)
{
    int semesterId = 1;

    std::vector<std::pair<int, std::string>> groups;
    if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
        std::cout << "У вас нет привязанных групп.\n";
        return;
    }

    std::cout << "\nВаши группы:\n";
    for (const auto& g : groups) {
        std::cout << " " << g.first << " - " << g.second << "\n";
    }

    int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
    if (groupId == 0) return;

    std::vector<std::pair<int, std::string>> students;
    if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
        std::cout << "В этой группе нет студентов.\n";
        return;
    }

    std::cout << "\nСтуденты группы:\n";
    for (const auto& st : students) {
        std::cout << " " << st.first << " - " << st.second << "\n";
    }

    int studentId = readIntInRange("Введите ID студента", 1, 1000000, students.front().first, true, 0);
    if (studentId == 0) return;

    int studentGroupId = 0, studentSubgroup = 0;
    if (!db.getStudentGroupAndSubgroup(studentId, studentGroupId, studentSubgroup)) {
        std::cout << "Ошибка: не удалось определить подгруппу студента.\n";
        return;
    }
    if (studentGroupId != groupId) {
        std::cout << "Ошибка: студент не из этой группы.\n";
        return;
    }

    int weekOfCycle = chooseWeekOfCycleOrDate(db);
    if (weekOfCycle == 0) {
        std::cout << "Некорректная неделя.\n";
        return;
    }

    sqlite3* rawdb = db.getHandle();
    const char* sql = R"(
        SELECT
sch.id,
               sch.subject_id,
               sch.weekday,
               sch.lesson_number,
               sch.sub_group,
               subj.name,
               sch.lesson_type
        FROM schedule sch
        JOIN subjects subj ON sch.subject_id = subj.id
        WHERE sch.teacher_id = ?
          AND sch.group_id = ?
          AND sch.week_of_cycle = ?
          AND (sch.sub_group = 0 OR sch.sub_group = ?)
        ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawdb, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "Ошибка SQL.\n";
        return;
    }

    sqlite3_bind_int(stmt, 1, getId());
    sqlite3_bind_int(stmt, 2, groupId);
    sqlite3_bind_int(stmt, 3, weekOfCycle);
    sqlite3_bind_int(stmt, 4, studentSubgroup);

    struct LessonInfo {
        int scheduleId;
        int subjectId;
        int weekday;
        int lessonNumber;
        int subgroup;
        std::string subjectName;
        std::string lessonType;
        std::string dateISO;
    };

    std::vector<LessonInfo> lessons;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LessonInfo info;
        info.scheduleId   = sqlite3_column_int(stmt, 0);
        info.subjectId    = sqlite3_column_int(stmt, 1);
        info.weekday      = sqlite3_column_int(stmt, 2);
        info.lessonNumber = sqlite3_column_int(stmt, 3);
        info.subgroup     = sqlite3_column_int(stmt, 4);

        const char* subjName = (const char*)sqlite3_column_text(stmt, 5);
        const char* ltype    = (const char*)sqlite3_column_text(stmt, 6);
        info.subjectName = subjName ? subjName : "";
        info.lessonType  = ltype ? ltype : "";

        if (!db.getDateForWeekday(weekOfCycle, info.weekday, info.dateISO)) {
            continue;
        }
        lessons.push_back(info);
    }
    sqlite3_finalize(stmt);

    if (lessons.empty()) {
        std::cout << "Нет доступных пар (подходящих по подгруппе).\n";
        return;
    }

    const char* pairTimes[] = {"08:30-09:55", "10:05-11:30", "12:00-13:25",
                               "13:35-15:00", "15:30-16:55", "17:05-18:30"};
    const char* dayNames[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};

    std::cout << "\nВыберите пару для пропуска:\n";
    for (size_t i = 0; i < lessons.size(); ++i) {
        const auto& les = lessons[i];

        std::string dateLabel;
        if (les.dateISO.size() == 10)
            dateLabel = les.dateISO.substr(8,2) + "-" + les.dateISO.substr(5,2);

        std::cout << (i + 1) << ") " << dayNames[les.weekday] << " " << dateLabel
                  << ", пара " << les.lessonNumber;
        if (les.lessonNumber >= 1 && les.lessonNumber <= 6)
            std::cout << " (" << pairTimes[les.lessonNumber - 1] << ")";
        if (les.subgroup == 1) std::cout << ", подгр. 1";
        else if (les.subgroup == 2) std::cout << ", подгр. 2";
        std::cout << ", " << les.subjectName << " [" << les.lessonType << "]\n";
    }

    int choice = readChoiceFromList("Номер пары", 1, (int)lessons.size(), true, 0);
    if (choice == 0) {
        std::cout << "Отмена.\n";
        return;
    }

    const auto& selected = lessons[choice - 1];

    // Проверка: уже есть оценка на эту дату и предмет?
    sqlite3_stmt* gradeStmt = nullptr;
    const char* gradeSQL =
        "SELECT COUNT(*) FROM grades "
        "WHERE student_id = ? AND subject_id = ? AND semester_id = ? AND date = ?";

    if (sqlite3_prepare_v2(rawdb, gradeSQL, -1, &gradeStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(gradeStmt, 1, studentId);
        sqlite3_bind_int(gradeStmt, 2, selected.subjectId);
        sqlite3_bind_int(gradeStmt, 3, semesterId);
        sqlite3_bind_text(gradeStmt, 4, selected.dateISO.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(gradeStmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(gradeStmt, 0);
            if (count > 0) {
                std::cout << "Ошибка: у студента уже есть оценка на эту пару ("
                          << selected.dateISO << "). Пропуск ставить нельзя.\n";
                sqlite3_finalize(gradeStmt);
                return;
            }
        }
        sqlite3_finalize(gradeStmt);
    }

    // 7. Ввод часов
    int hours = readIntInRange("Количество часов пропуска (1 или 2)", 1, 2, 1, true, 0);
    if (hours == 0) {
        std::cout << "Отмена.\n";
        return;
    }

    // 8. Тип пропуска
    int typeChoice = readIntInRange("Тип пропуска (1-уважительный, 2-неуважительный)", 1, 2, 1, true, 0);
    if (typeChoice == 0) {
        std::cout << "Отмена.\n";
        return;
    }
    std::string type = (typeChoice == 1) ? "excused" : "unexcused";

    // 9. Добавить пропуск
    if (db.addAbsence(studentId, selected.subjectId, semesterId, hours, selected.dateISO, type)) {
        std::cout << "✓ Пропуск добавлен: " << hours << " ч. на " << selected.dateISO
                  << ", предмет: " << selected.subjectName << " [" << type << "]\n";
    } else {
        std::cout << "Ошибка при добавлении пропуска.\n";
    }
}
void Teacher::viewStudentAbsences(Database& db, int semesterId)
{
    std::vector<std::pair<int, std::string>> groups;
    if (!db.getGroupsForTeacher(getId(), groups) || groups.empty()) {
        std::cout << "У вас нет привязанных групп.\n";
        return;
    }

    std::cout << "\nВаши группы:\n";
    for (const auto& g : groups) {
        std::cout << " " << g.first << " - " << g.second << "\n";
    }

    int groupId = readIntInRange("Введите id группы", 1, 1000000, groups.front().first, true, 0);
    if (groupId == 0) return;

    std::vector<std::pair<int, std::string>> students;
    if (!db.getStudentsOfGroup(groupId, students) || students.empty()) {
        std::cout << "В этой группе нет студентов.\n";
        return;
    }

    std::cout << "\nСтуденты группы " << groupId << ":\n";
    for (const auto& st : students) {
        std::cout << " " << st.first << " - " << st.second << "\n";
    }

    int studentId = readIntInRange("Введите ID студента", 1, 1000000, students.front().first, true, 0);
    if (studentId == 0) return;

    std::vector<std::tuple<std::string,int,std::string,std::string>> absences;
    if (!db.getStudentAbsencesForSemester(studentId, semesterId, absences)) {
        std::cout << "Ошибка при получении пропусков.\n";
        return;
    }

    if (absences.empty()) {
        std::cout << "Пропусков нет.\n";
        return;
    }

    std::cout << "\nПропуски студента " << studentId << ":\n";
    std::cout << "Дата | предмет | часы | тип\n";
    std::cout << "─────────────────────────────────────────\n";

    int total = 0;
    for (const auto& a : absences) {
        const std::string& subj = std::get<0>(a);
        int hours = std::get<1>(a);
        const std::string& date = std::get<2>(a);
        const std::string& type = std::get<3>(a);

        total += hours;
        std::cout << date << " | " << subj << " | "
                  << hours << " часов | " << type << "\n";
    }

    std::cout << "─────────────────────────────────────────\n";
    std::cout << "Всего часов пропусков: " << total << "\n";
}


// ===== НОВЫЕ ФУНКЦИИ РАСПИСАНИЯ ДЛЯ ПРЕПОДАВАТЕЛЯ =====
bool Teacher::viewMySchedule(int week_of_cycle)
{
    if (!db_ || !db_->isConnected()) {
        std::cerr << "[✗] БД не инициализирована\n";
        return false;
    }

    sqlite3* raw_db = db_->getHandle();
    const char* sql = R"(
        SELECT
            sch.lesson_number, -- 0
            sch.weekday,      -- 1
            g.name AS group_name, -- 2
            subj.name AS subject, -- 3
            sch.room,         -- 4
            sch.sub_group,     -- 5
            sch.lesson_type    -- 6
        FROM schedule sch
        JOIN subjects subj ON sch.subject_id = subj.id
        JOIN groups g ON sch.group_id = g.id
        WHERE sch.teacher_id = ?
          AND sch.week_of_cycle = ?
        ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(raw_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка подготовки запроса: " << sqlite3_errmsg(raw_db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacher_id_);
    sqlite3_bind_int(stmt, 2, week_of_cycle);

    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ МОЁ РАСПИСАНИЕ - НЕДЕЛЯ " << week_of_cycle << "\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    const char* dayNames[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"};
    const char* pairTimes[] = {
        "08:30-09:55",
        "10:05-11:30",
        "12:00-13:25",
        "13:35-15:00",
        "15:30-16:55",
        "17:05-18:30"
    };

    int currentDay = -1;
    bool found = false;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;

        int lessonNumber = sqlite3_column_int(stmt, 0);
        int weekday      = sqlite3_column_int(stmt, 1);
        const char* group   = (const char*)sqlite3_column_text(stmt, 2);
        const char* subject = (const char*)sqlite3_column_text(stmt, 3);
        const char* room    = (const char*)sqlite3_column_text(stmt, 4);
        int subgroup        = sqlite3_column_int(stmt, 5);
        const char* ltype   = (const char*)sqlite3_column_text(stmt, 6);

        if (weekday != currentDay) {
            currentDay = weekday;

            std::string dateISO;
            std::string dateLabel;
            if (db_->getDateForWeekday(week_of_cycle, weekday, dateISO) && dateISO.size() == 10) {
                dateLabel = dateISO.substr(8, 2) + "-" + dateISO.substr(5, 2);
            }

            if (weekday >= 0 && weekday <= 5) {
                if (!dateLabel.empty()) std::cout << "\n[" << dayNames[weekday] << "] (" << dateLabel << ")\n";
                else std::cout << "\n[" << dayNames[weekday] << "]\n";
            } else {
                std::cout << "\n[день " << weekday << "]\n";
            }
        }

        std::string subjectText = subject ? subject : "";
        if (subgroup == 1) subjectText += " (подгр. 1)";
        else if (subgroup == 2) subjectText += " (подгр. 2)";

        std::string typeLabel;
        if (ltype && *ltype) typeLabel = " [" + std::string(ltype) + "]";

        std::cout << " " << lessonNumber << "-я пара";
        if (lessonNumber >= 1 && lessonNumber <= 6) {
            std::cout << " (" << pairTimes[lessonNumber - 1] << ")";
        }

        std::cout << " | " << (group ? group : "") << " | "
                  << subjectText << typeLabel << " | "
                  << (room ? room : "") << "\n";
    }

    sqlite3_finalize(stmt);

    if (!found) {
        std::cout << "Расписание не найдено на эту неделю.\n";
    }

    return true;
}

bool Teacher::viewFullSchedule()
{
    if (!db_) {
        std::cerr << "[✗] БД не инициализирована\n";
        return false;
    }

    for (int week = 1; week <= 4; ++week) {
        std::cout << "\n══════════════════ НЕДЕЛЯ " << week << " ══════════════════\n";
        viewMySchedule(week);
    }
    return true;
}


