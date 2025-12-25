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
    sqlite3* db;
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
    sqlite3* rawHandle() const { return db; }
    bool getAllSemesters(std::vector<std::pair<int, std::string>>& outSemesters);
    bool getSubjectsForTeacher(int teacherId,
                           std::vector<std::pair<int, std::string>>& outSubjects);

    bool getSubjectsForTeacherInGroupSchedule(int teacherId, int groupId,
                           std::vector<std::pair<int, std::string>>& outSubjects);

    bool getAllGroups(std::vector<std::pair<int, std::string>>& outGroups);
    bool getAllUsers(
        std::vector<std::tuple<int, std::string, std::string, std::string, int, int>>& outUsers
    );
    bool getStudentGradesForSemester(int studentId, int semesterId,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outGrades);
    // (subjectName, value, date, grade_type)

    bool getStudentsOfGroup(int groupId,
                            std::vector<std::pair<int, std::string>>& outStudents);
    bool getStudentSubjectGrades(int studentId,
                                 int subjectId,
                                 int semesterId,
                                 std::vector<std::tuple<int, std::string, std::string>>& outGrades);
    bool addGrade(int studentId, int subjectId, int semesterId,
              int value, const std::string& date, const std::string& gradeType = "");

    bool insertUser(const std::string& username,
                const std::string& password,
                const std::string& role,
                const std::string& name,
                int groupId,
                int subgroup);
    // для admin/teacher можно передавать 0
    bool deleteUserById(int userId);
    // Правильное объявление (строка ~87, оставь это):
    bool updateGrade(int gradeId, int newValue, const std::string& newDate,
                     const std::string& newType);

    // УДАЛИ дубликат на строке 188, если он там есть!

    bool getGradesForStudentSubject(int studentId,
                                    int subjectId,
                                    int semesterId,
                                    std::vector<std::tuple<int, int, std::string, std::string>>& outGrades);

    // ===== МЕТОДЫ ДЛЯ ПРОПУСКОВ =====
    bool addAbsence(int studentId,
                int subjectId,
                int semesterId,
                int hours,
                const std::string& date,
                const std::string& type);

    // ===== Teacher journal helpers (GUI) =====
    // Find existing grade by (student, subject, semester, date). Returns true on SQL success.
    bool findGradeId(int studentId, int subjectId, int semesterId,
                     const std::string& date, int& outGradeId);

    // Find existing absence by (student, subject, semester, date). Returns true on SQL success.
    bool findAbsenceId(int studentId, int subjectId, int semesterId,
                       const std::string& date, int& outAbsenceId);

    // Update grade if exists for key, otherwise insert. Returns true on success.
    bool upsertGradeByKey(int studentId, int subjectId, int semesterId,
                          int value, const std::string& date, const std::string& gradeType = "");

    bool getGradeById(int gradeId, int& outValue, std::string& outDate, std::string& outGradeType);

    // Update absence if exists for key, otherwise insert. Returns true on success.
    bool upsertAbsenceByKey(int studentId, int subjectId, int semesterId,
                            int hours, const std::string& date, const std::string& type);

    bool getAbsenceById(int absenceId, int& outHours, std::string& outDate, std::string& outType);
    bool deleteAbsence(int absenceId);

    bool getStudentAbsencesForSemester(
    int studentId,
    int semesterId,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outAbsences);
    // (subject_name, hours, date, type)

    bool getStudentTotalAbsences(int studentId,
                                 int semesterId,
                                 int& outTotalHours);

    bool getStudentUnexcusedAbsences(int studentId,
                             int semesterId,
                             int& outUnexcusedHours);

    bool deleteTodayAbsence(int studentId,
                        int subjectId,
                        int semesterId,
                        const std::string& date);
    bool addTeacherGroup(int teacherId, int groupId);

    bool getGroupsForTeacher(int teacherId,
                             std::vector<std::pair<int, std::string>>& outGroups);
    // ===== РАСПИСАНИЕ ПАР (LESSONS) =====
    bool addLesson(int groupId,
                   int subjectId,
                   int teacherId,
                   const std::string& date,
                   int timeSlot );

    // Получить пары группы на конкретную дату
    bool getLessonsForGroupAndDate(int groupId,
                                   const std::string& date,
                                   std::vector<std::tuple<int,int,std::string>>& outLessons);
    // outLessons: (time_slot, subject_id, subject_name)
    bool getGroupSubjectAbsencesSummary(
        int groupId,
        int subjectId,
        int semesterId,
        std::vector<std::tuple<int, std::string, int>>& outRows);
    // (student_id, student_name, total_hours)
    bool addScheduleEntry(int groupId, int subgroup, int weekday,
                          int lessonNumber, int weekOfCycle,
                          int subjectId, int teacherId,
                          const std::string& room);

    bool getScheduleForGroup(
     int groupId,
     int weekday,
     int weekOfCycle,
     std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>& rows
 );





    // уже добавленные раньше:
    bool getAllSubjects(std::vector<std::pair<int, std::string>>& outSubjects);
    bool getAllTeachers(std::vector<std::pair<int, std::string>>& outTeachers);
    bool getStudentGroupAndSubgroup(int studentId, int& outGroupId, int& outSubgroup);
    bool getScheduleForTeacherGroup(
    int teacherId,
    int groupId,
    std::vector<std::tuple<int,int,int,int,int,std::string>>& rows
);

    bool isScheduleEmpty(bool& outEmpty);

    void dumpDbStats();
    void dumpSchemaAndCounts();
    // Загрузить расписание из SQL файла
    bool loadScheduleFromFile(const std::string& filePath);

    // Загрузить расписание конкретной группы
    bool loadGroupSchedule(int groupId, const std::string& filePath);

    sqlite3* getHandle() { return db; }
    sqlite3* getRawHandle() const { return db; }
    int  getWeekOfCycleForDate(const std::string& dateISO);
    bool getCycleWeeks(std::vector<std::tuple<int,int,std::string,std::string>>& out);
    bool getDateForWeekdayByWeekId(int weekId, int weekday, std::string& outDateISO);
    int getWeekOfCycleByWeekId(int weekId);
    bool getDateForWeekday(int weekOfCycle, int weekday, std::string& outDateISO);
    // Проверить, не занята ли ячейка расписания (группа+день+пара+неделя+подгруппа)
    bool isScheduleSlotBusy(int groupId, int subgroup,
                            int weekday, int lessonNumber, int weekOfCycle);

    // Обновить существующую запись расписания по id

    // Удалить запись расписания по id
    bool addScheduleEntry(int groupId, int subgroup, int weekday, int lessonNumber,
                      int weekOfCycle, int subjectId, int teacherId,
                      const std::string& room, const std::string& lessonType );

// Удалить запись расписания по id
bool deleteScheduleEntry(int scheduleId);
    // Получить расписание для группы на неделю (для редактирования)
    // возвращает: id, weekday, lesson_number, sub_group, subject_name, teacher_name, room, lesson_type
    // scheduleId, subjectId, weekday, lessonNumber, subgroup, subjectName, lessonType
    bool getScheduleForTeacherGroupWeek(
        int teacherId,
        int groupId,
        int weekOfCycle,
        int studentSubgroup,
        std::vector<std::tuple<int,int,int,int,int,std::string,std::string>>& outRows
    );

    // То же, что getScheduleForTeacherGroupWeek, но с аудиториями (room)
    // outRows: scheduleId, subjectId, weekday, lessonNumber, subgroup, subjectName, room, lessonType
    bool getScheduleForTeacherGroupWeekWithRoom(
        int teacherId,
        int groupId,
        int weekOfCycle,
        int studentSubgroup,
        std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string>>& outRows
    );

    // Расписание преподавателя за неделю по всем группам
    // outRows: scheduleId, groupId, weekday, lessonNumber, subgroup, subjectName, room, lessonType, groupName
    bool getScheduleForTeacherWeekWithRoom(
        int teacherId,
        int weekOfCycle,
        int studentSubgroup,
        std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string,std::string>>& outRows
    );

    // Добавить запись расписания для всех групп, кроме basegroup_id, если это лекция
    bool addLectureForAllGroups(int basegroupId, int subgroup,
                                int weekday, int lessonNumber, int weekOfCycle,
                                int subjectId, int teacherId,
                                const std::string& room, const std::string& lessonType );
    // ===== НОВЫЕ МЕТОДЫ ДЛЯ ПРОВЕРКИ КОНФЛИКТОВ =====

    // Проверка занятости преподавателя в указанный слот
    bool isTeacherBusy(int teacherId, int weekday, int lessonNumber, int weekOfCycle);

    // Проверка занятости аудитории в указанный слот
    bool isRoomBusy(const std::string& room, int weekday, int lessonNumber, int weekOfCycle);

    // Получить список преподавателей с их предметами (для удобного выбора)
    // Возвращает: (teacher_id, teacher_name, "Предмет1, Предмет2, ...")
    bool getTeachersWithSubjects(
        std::vector<std::tuple<int, std::string, std::string>>& outTeachers);
    bool deleteGrade(int gradeId);
    // Где-то рядом с другими методами расписания добавь:
    bool updateScheduleEntry(int scheduleId, int groupId, int subgroup, int weekday,
                             int lessonNumber, int weekOfCycle, int subjectId,
                             int teacherId, const std::string& room, const std::string& lessonType);
    int getWeekIdByDate(const std::string& dateISO);


};



#endif // DATABASE_H
