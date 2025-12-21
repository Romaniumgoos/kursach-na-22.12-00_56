#pragma once

class Database;

class Statistics {
public:
    static double calculateStudentAverage(Database& db,
                                          int studentId,
                                          int semesterId);

    static double calculateStudentSubjectAverage(Database& db,
                                                 int studentId,
                                                 int subjectId,
                                                 int semesterId);

    static double calculateGroupSubjectAverage(Database& db,
                                               int groupId,
                                               int subjectId,
                                               int semesterId);
};
