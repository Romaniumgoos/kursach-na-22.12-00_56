#include "services/student_service.h"


Result<std::pair<int,int>>
StudentService::getStudentGroupAndSubgroup(int studentId) {
    int groupId = 0, subgroup = 0;
    if (!db_.getStudentGroupAndSubgroup(studentId, groupId, subgroup))
        return Result<std::pair<int,int>>::Fail("DB: getStudentGroupAndSubgroup failed");
    return Result<std::pair<int,int>>::Ok({groupId, subgroup});
}

Result<std::vector<std::tuple<std::string,int,std::string,std::string>>>
StudentService::getGradesForSemester(int studentId, int semesterId) {
    std::vector<std::tuple<std::string,int,std::string,std::string>> grades;
    if (!db_.getStudentGradesForSemester(studentId, semesterId, grades))
        return Result<decltype(grades)>::Fail("DB: getStudentGradesForSemester failed");
    return Result<decltype(grades)>::Ok(std::move(grades));
}

Result<std::vector<std::tuple<std::string,int,std::string,std::string>>>
StudentService::getAbsencesForSemester(int studentId, int semesterId) {
    std::vector<std::tuple<std::string,int,std::string,std::string>> absences;
    if (!db_.getStudentAbsencesForSemester(studentId, semesterId, absences))
        return Result<decltype(absences)>::Fail("DB: getStudentAbsencesForSemester failed");
    return Result<decltype(absences)>::Ok(std::move(absences));
}

Result<std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>>
StudentService::getScheduleForGroup(int groupId, int weekday, int weekOfCycle) {
    std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> rows;
    if (!db_.getScheduleForGroup(groupId, weekday, weekOfCycle, rows))
        return Result<decltype(rows)>::Fail("DB: getScheduleForGroup failed");
    return Result<decltype(rows)>::Ok(std::move(rows));
}

Result<std::string>
StudentService::getDateISO(int weekOfCycle, int weekday) {
    std::string dateISO;
    if (!db_.getDateForWeekday(weekOfCycle, weekday, dateISO))
        return Result<std::string>::Fail("DB: getDateForWeekday failed");
    return Result<std::string>::Ok(std::move(dateISO));
}
