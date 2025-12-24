#pragma once

#include "core/result.h"
#include "database.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>
struct LessonDto {
    int    id;
    int    weekday;       // 0..5
    int    pairNumber;    // 1..6
    int    subgroup;      // 0,1,2
    std::string subject;
    std::string teacher;
    std::string room;
    std::string lessonType; // "ЛК"/"ПЗ"/"ЛР" или ""
    std::string dateISO;    // "YYYY-MM-DD"
};

struct GradeDto {
    int    id;
    int    value;
    std::string subject;
    std::string dateISO;
    std::string type;  // можно оставить "" если не используешь
};

struct AbsenceDto {
    int    id;    // можно не хранить, если в БД нет
    int    hours;
    std::string subject;
    std::string dateISO;
    std::string type; // "excused"/"unexcused"
};

class StudentService {
public:
    using GroupAndSubgroup = std::pair<int, int>; // (groupId, subgroup)

    // (subjectName, gradeValue, dateISO, gradeType)
    using GradeRow = std::tuple<std::string, int, std::string, std::string>;

    // (subjectName, hours, dateISO, absenceType)
    using AbsenceRow = std::tuple<std::string, int, std::string, std::string>;

    // schedule rows: (id, lessonNumber, subgroup, subject, room, lessonType, teacher)
    using ScheduleRow =
        std::tuple<int, int, int, std::string, std::string, std::string, std::string>;

public:
    explicit StudentService(Database& db) : db_(db) {}

    [[nodiscard]] Result<GroupAndSubgroup> getStudentGroupAndSubgroup(int studentId);

    [[nodiscard]] Result<std::vector<GradeRow>> getGradesForSemester(int studentId, int semesterId);

    [[nodiscard]] Result<std::vector<AbsenceRow>> getAbsencesForSemester(int studentId, int semesterId);

    [[nodiscard]] Result<std::vector<ScheduleRow>> getScheduleForGroup(int groupId, int weekday, int weekOfCycle);

    [[nodiscard]] Result<std::string> getDateISO(int weekOfCycle, int weekday);

private:
    Database& db_;
};
