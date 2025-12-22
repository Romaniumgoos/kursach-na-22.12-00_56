#pragma once
#include "core/result.h"
#include "database.h"

#include <tuple>
#include <vector>
#include <string>
#include <utility>

class StudentService {
public:
    explicit StudentService(Database& db) : db_(db) {}

    Result<std::pair<int,int>> getStudentGroupAndSubgroup(int studentId); // (groupId, subgroup)

    Result<std::vector<std::tuple<std::string,int,std::string,std::string>>>
    getGradesForSemester(int studentId, int semesterId);

    Result<std::vector<std::tuple<std::string,int,std::string,std::string>>>
    getAbsencesForSemester(int studentId, int semesterId);

    // schedule rows: (id, lessonNumber, subgroup, subject, room, lessonType, teacher)
    Result<std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>>
    getScheduleForGroup(int groupId, int weekday, int weekOfCycle);

    Result<std::string> getDateISO(int weekOfCycle, int weekday);

private:
    Database& db_;
};
