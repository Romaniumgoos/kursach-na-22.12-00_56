#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <sqlite3.h>
#include <vector>
#include <utility>
#include <tuple>


// ============================================================
// КЛАСС ДЛЯ РАБОТЫ С БАЗОЙ ДАННЫХ (SQLite)
// ============================================================
// Этот класс отвечает за:
// 1. Открытие файла БД SQLite
// 2. Выполнение SQL-запросов без выборки (CREATE, INSERT, UPDATE, DELETE)
// 3. Проверку, открыта ли БД
//
// Важно:
// - Вместо сервера MySQL используется обычный файл, например "students.db".
// - Другие классы не знают, что внутри именно SQLite.

class Database {
private:
    sqlite3* db_;
    std::string fileName;


public:
    // Конструктор - запоминает имя файла БД (например, "students.db")
    explicit Database(const std::string& file);

    // Деструктор - закрывает БД при удалении объекта
    ~Database();

    // Открыть БД (если файла нет, SQLite может создать его)
    // Возвращает true, если успешно, false при ошибке
    bool connect();

    // Проверить, открыта ли БД
    bool isConnected() const;

    // Закрыть БД
    void disconnect();

    // Выполнить SQL без выборки (CREATE TABLE, INSERT, UPDATE, DELETE)
    bool execute(const std::string& sql);

    bool initialize();
    bool initializeDemoData();  // ← новый метод для наполнения БД
    bool findUser(const std::string& username,
              const std::string& password,
              int& outId,
              std::string& outName,
              std::string& outRole);


    // Доступ к "сырому" указателю sqlite3*, если понадобится позже
    sqlite3* rawHandle() const { return db_; }
    bool getAllSemesters(std::vector<std::pair<int, std::string>>& outSemesters);
    bool getSubjectsForTeacher(int teacher_id,
                           std::vector<std::pair<int, std::string>>& outSubjects);

    bool getAllGroups(std::vector<std::pair<int, std::string>>& outGroups);
    bool getAllUsers(
        std::vector<std::tuple<int, std::string, std::string, std::string, int, int>>& outUsers
    );
    bool getStudentGradesForSemester(int student_id, int semester_id,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& out_grades);
    // (subjectName, value, date, grade_type)

    bool getStudentsOfGroup(int group_id,
                            std::vector<std::pair<int, std::string>>& outStudents);
    bool getStudentSubjectGrades(int student_id,
                                 int subject_id,
                                 int semester_id,
                                 std::vector<std::tuple<int, std::string, std::string>>& outGrades);
    bool addGrade(int student_id, int subject_id, int semester_id,
              int value, const std::string& date, const std::string& grade_type = "");

    bool insertUser(const std::string& username,
                const std::string& password,
                const std::string& role,
                const std::string& name,
                int group_id,
                int sub_group);
    // для admin/teacher можно передавать 0
    bool deleteUserById(int userId);
    // Правильное объявление (строка ~87, оставь это):
    bool updateGrade(int grade_id, int newValue, const std::string& newDate,
                     const std::string& newType);

    // УДАЛИ дубликат на строке 188, если он там есть!

    bool getGradesForStudentSubject(int student_id,
                                    int subject_id,
                                    int semester_id,
                                    std::vector<std::tuple<int, int, std::string, std::string>>& outGrades);

    // ===== МЕТОДЫ ДЛЯ ПРОПУСКОВ =====
    bool addAbsence(int student_id,
                int subject_id,
                int semester_id,
                int hours,
                const std::string& date,
                const std::string& type);

    bool getStudentAbsencesForSemester(
    int student_id,
    int semester_id,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outAbsences);
    // (subject_name, hours, date, type)

    bool getStudentTotalAbsences(int student_id,
                                 int semester_id,
                                 int& outTotalHours);

    bool getStudentUnexcusedAbsences(int student_id,
                             int semester_id,
                             int& outUnexcusedHours);

    bool deleteTodayAbsence(int student_id,
                        int subject_id,
                        int semester_id,
                        const std::string& date);
    bool addTeacherGroup(int teacher_id, int group_id);

    bool getGroupsForTeacher(int teacher_id,
                             std::vector<std::pair<int, std::string>>& outGroups);
    // ===== РАСПИСАНИЕ ПАР (LESSONS) =====
    bool addLesson(int group_id,
                   int subject_id,
                   int teacher_id,
                   const std::string& date,
                   int time_slot );

    // Получить пары группы на конкретную дату
    bool getLessonsForGroupAndDate(int group_id,
                                   const std::string& date,
                                   std::vector<std::tuple<int,int,std::string>>& outLessons);
    // outLessons: (time_slot, subject_id, subject_name)
    bool getGroupSubjectAbsencesSummary(
        int group_id,
        int subject_id,
        int semester_id,
        std::vector<std::tuple<int, std::string, int>>& outRows);
    // (student_id, student_name, total_hours)
    bool addScheduleEntry(int group_id, int sub_group, int weekday,
                          int lesson_number, int week_of_cycle,
                          int subject_id, int teacher_id,
                          const std::string& room);

    bool getScheduleForGroup(
     int group_id,
     int weekday,
     int week_of_cycle,
     std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>& rows
 );





    // уже добавленные раньше:
    bool getAllSubjects(std::vector<std::pair<int, std::string>>& outSubjects);
    bool getAllTeachers(std::vector<std::pair<int, std::string>>& outTeachers);
    bool getStudentGroupAndSubgroup(int student_id, int& out_group_id, int& out_sub_group);
    bool getScheduleForTeacherGroup(
    int teacher_id,
    int group_id,
    std::vector<std::tuple<int,int,int,int,int,std::string>>& rows
);

    bool isScheduleEmpty(bool& outEmpty);
    // Загрузить расписание из SQL файла
    bool loadScheduleFromFile(const std::string& filePath);

    // Загрузить расписание конкретной группы
    bool loadGroupSchedule(int group_id, const std::string& filePath);

    sqlite3* getHandle() { return db_; }
    sqlite3* getRawHandle() const { return db_; }
    //int  getweekofcycleForDate(const std::string& dateISO);
    bool getCycleWeeks(std::vector<std::tuple<int,int,std::string,std::string>>& out);
    int getWeekOfCycleByDate(const std::string& dateISO);
    bool getDateForWeekday(int week_of_cycle, int weekday, std::string& outDateISO);
    // Проверить, не занята ли ячейка расписания (группа+день+пара+неделя+подгруппа)
    bool isScheduleSlotBusy(int group_id, int sub_group,
                            int weekday, int lesson_number, int week_of_cycle);

    // Обновить существующую запись расписания по id

    // Удалить запись расписания по id
    bool addScheduleEntry(int group_id, int sub_group, int weekday, int lesson_number,
                      int week_of_cycle, int subject_id, int teacher_id,
                      const std::string& room, const std::string& lesson_type );

// Удалить запись расписания по id
bool deleteScheduleEntry(int scheduleId);
    // Получить расписание для группы на неделю (для редактирования)
    // возвращает: id, weekday, lesson_number, sub_group, subject_name, teacher_name, room, lesson_type
    bool getScheduleForGroupWeek(
        int group_id, int week_of_cycle,
        std::vector<std::tuple<int,int,int,int,
                               std::string,std::string,
                               std::string,std::string>>& rows);
    // Добавить запись расписания для всех групп, кроме basegroup_id, если это лекция
    bool addLectureForAllGroups(int basegroup_id, int sub_group,
                                int weekday, int lesson_number, int week_of_cycle,
                                int subject_id, int teacher_id,
                                const std::string& room, const std::string& lesson_type );
    // ===== НОВЫЕ МЕТОДЫ ДЛЯ ПРОВЕРКИ КОНФЛИКТОВ =====

    // Проверка занятости преподавателя в указанный слот
    bool isTeacherBusy(int teacher_id, int weekday, int lesson_number, int week_of_cycle);

    // Проверка занятости аудитории в указанный слот
    bool isRoomBusy(const std::string& room, int weekday, int lesson_number, int week_of_cycle);

    // Получить список преподавателей с их предметами (для удобного выбора)
    // Возвращает: (teacher_id, teacher_name, "Предмет1, Предмет2, ...")
    bool getTeachersWithSubjects(
        std::vector<std::tuple<int, std::string, std::string>>& outTeachers);
    bool deleteGrade(int grade_id);
    // Где-то рядом с другими методами расписания добавь:
    bool updateScheduleEntry(int schedule_id, int group_id, int sub_group, int weekday,
                             int lesson_number, int week_of_cycle, int subject_id,
                             int teacher_id, const std::string& room, const std::string& lesson_type);

};



#endif // DATABASE_H
