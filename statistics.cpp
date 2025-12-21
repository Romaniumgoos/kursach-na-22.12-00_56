#include "statistics.h"
#include "database.h"

#include <sqlite3.h>
#include <iostream>

double Statistics::calculateStudentAverage(Database& db,
                                           int studentId,
                                           int semesterId)
{
    sqlite3* handle = db.rawHandle();
    if (!handle) {
        std::cerr << "[✗] Statistics: нет соединения с БД\n";
        return 0.0;
    }

    const char* sql =
        "SELECT AVG(value) "
        "FROM grades "
        "WHERE student_id = ? AND semester_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Statistics: prepare error: "
                  << sqlite3_errmsg(handle) << "\n";
        return 0.0;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, semesterId);

    double avg = 0.0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        avg = sqlite3_column_double(stmt, 0);
    } else if (rc != SQLITE_DONE) {
        std::cerr << "[✗] Statistics: step error: "
                  << sqlite3_errmsg(handle) << "\n";
    }

    sqlite3_finalize(stmt);
    return avg;
}

double Statistics::calculateStudentSubjectAverage(Database& db,
                                                  int studentId,
                                                  int subjectId,
                                                  int semesterId)
{
    sqlite3* handle = db.rawHandle();
    if (!handle) {
        std::cerr << "[✗] Statistics: нет соединения с БД\n";
        return 0.0;
    }

    const char* sql =
        "SELECT AVG(value) "
        "FROM grades "
        "WHERE student_id = ? AND subject_id = ? AND semester_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Statistics: prepare error: "
                  << sqlite3_errmsg(handle) << "\n";
        return 0.0;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);

    double avg = 0.0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        avg = sqlite3_column_double(stmt, 0);
    } else if (rc != SQLITE_DONE) {
        std::cerr << "[✗] Statistics: step error: "
                  << sqlite3_errmsg(handle) << "\n";
    }

    sqlite3_finalize(stmt);
    return avg;
}

double Statistics::calculateGroupSubjectAverage(Database& db,
                                                int groupId,
                                                int subjectId,
                                                int semesterId)
{
    sqlite3* handle = db.rawHandle();
    if (!handle) {
        std::cerr << "[✗] Statistics: нет соединения с БД\n";
        return 0.0;
    }

    const char* sql =
        "SELECT AVG(g.value) "
        "FROM grades g "
        "JOIN users u ON g.student_id = u.id "
        "WHERE u.group_id = ? AND g.subject_id = ? AND g.semester_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Statistics: prepare error: "
                  << sqlite3_errmsg(handle) << "\n";
        return 0.0;
    }

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);

    double avg = 0.0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        avg = sqlite3_column_double(stmt, 0);
    } else if (rc != SQLITE_DONE) {
        std::cerr << "[✗] Statistics: step error: "
                  << sqlite3_errmsg(handle) << "\n";
    }

    sqlite3_finalize(stmt);
    return avg;
}
