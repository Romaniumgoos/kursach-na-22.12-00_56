#include "student_service.h"

Result<StudentService::GroupAndSubgroup>
StudentService::getStudentGroupAndSubgroup(int studentId) {
    if (studentId <= 0) {
        return Result<GroupAndSubgroup>::Fail("studentId must be > 0");
    }

    int groupId = 0;
    int subgroup = 0;

    if (!db.getStudentGroupAndSubgroup(studentId, groupId, subgroup)) {
        return Result<GroupAndSubgroup>::Fail("DB: getStudentGroupAndSubgroup failed");
    }

    return Result<GroupAndSubgroup>::Ok({groupId, subgroup});
}

Result<std::vector<StudentService::GradeRow>>
StudentService::getGradesForSemester(int studentId, int semesterId) {
    if (studentId <= 0) {
        return Result<std::vector<GradeRow>>::Fail("studentId must be > 0");
    }
    if (semesterId <= 0) {
        return Result<std::vector<GradeRow>>::Fail("semesterId must be > 0");
    }

    std::vector<GradeRow> grades;
    if (!db.getStudentGradesForSemester(studentId, semesterId, grades)) {
        return Result<std::vector<GradeRow>>::Fail("DB: getStudentGradesForSemester failed");
    }

    return Result<std::vector<GradeRow>>::Ok(std::move(grades));
}

Result<std::vector<StudentService::AbsenceRow>>
StudentService::getAbsencesForSemester(int studentId, int semesterId) {
    if (studentId <= 0) {
        return Result<std::vector<AbsenceRow>>::Fail("studentId must be > 0");
    }
    if (semesterId <= 0) {
        return Result<std::vector<AbsenceRow>>::Fail("semesterId must be > 0");
    }

    std::vector<AbsenceRow> absences;
    if (!db.getStudentAbsencesForSemester(studentId, semesterId, absences)) {
        return Result<std::vector<AbsenceRow>>::Fail("DB: getStudentAbsencesForSemester failed");
    }

    return Result<std::vector<AbsenceRow>>::Ok(std::move(absences));
}

Result<std::vector<StudentService::ScheduleRow>>
StudentService::getScheduleForGroup(int groupId, int weekday, int weekOfCycle) {
    if (groupId < 0) {
        return Result<std::vector<ScheduleRow>>::Fail("groupId must be >= 0");
    }
    // В твоём проекте дни часто идут как 0..5 (Пн..Сб). Если у тебя 0..6 — скажи, поменяю.
    if (weekday < 0 || weekday > 5) {
        return Result<std::vector<ScheduleRow>>::Fail("weekday must be in [0..5]");
    }
    if (weekOfCycle < 1 || weekOfCycle > 4) {
        return Result<std::vector<ScheduleRow>>::Fail("weekOfCycle must be in [1..4]");
    }

    std::vector<ScheduleRow> rows;
    if (!db.getScheduleForGroup(groupId, weekday, weekOfCycle, rows)) {
        return Result<std::vector<ScheduleRow>>::Fail("DB: getScheduleForGroup failed");
    }

    return Result<std::vector<ScheduleRow>>::Ok(std::move(rows));
}

Result<std::string>
StudentService::getDateISO(int weekOfCycle, int weekday) {
    if (weekday < 0 || weekday > 5) {
        return Result<std::string>::Fail("weekday must be in [0..5]");
    }
    if (weekOfCycle < 1 || weekOfCycle > 4) {
        return Result<std::string>::Fail("weekOfCycle must be in [1..4]");
    }

    std::string dateISO;
    if (!db.getDateForWeekday(weekOfCycle, weekday, dateISO)) {
        return Result<std::string>::Fail("DB: getDateForWeekday failed");
    }

    return Result<std::string>::Ok(std::move(dateISO));
}

