#include "database.h"
#include <sqlite3.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <ctime>
#include <cstring>
#include <cctype>

namespace {

// Helper: Get ID by single text parameter
int getIdBySingleTextParam(sqlite3* db, const char* sql, const std::string& value) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_TRANSIENT);
    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return id;
}

int getUserIdByUsername(sqlite3* db, const std::string& username) {
    return getIdBySingleTextParam(db,
        "SELECT id FROM users WHERE username = ? LIMIT 1;",
        username);
}

int getGroupIdByName(sqlite3* db, const std::string& name) {
    return getIdBySingleTextParam(db,
        "SELECT id FROM groups WHERE name = ? LIMIT 1;",
        name);
}

// Parse ISO date (YYYY-MM-DD) to std::tm
bool parseDateISO(const std::string& s, std::tm& out) {
    if (s.size() != 10) return false;
    memset(&out, 0, sizeof(out));
    out.tm_year = std::stoi(s.substr(0, 4)) - 1900;
    out.tm_mon = std::stoi(s.substr(5, 2)) - 1;   // 0–11
    out.tm_mday = std::stoi(s.substr(8, 2));      // 1–31
    out.tm_hour = 0;
    out.tm_min = 0;
    out.tm_sec = 0;
    std::mktime(&out);
    return true;
}

} // namespace

bool Database::resolveWeekdayZeroBased()
{
    if (weekdayZeroBasedResolved) return true;
    weekdayZeroBasedResolved = true;
    weekdayZeroBased = false;

    if (!db) return false;

    const char* sql = "SELECT MIN(weekday), MAX(weekday) FROM schedule;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // If table is empty, MIN/MAX are NULL.
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            const int minW = sqlite3_column_int(stmt, 0);
            // Heuristic:
            // - if 0 is present -> old convention (0..5)
            // - otherwise assume 1..6
            if (minW == 0) weekdayZeroBased = true;
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

int Database::normalizeWeekdayForDb(int weekday)
{
    resolveWeekdayZeroBased();

    // UI & most GUI code use 1..6 (Mon..Sat). Some legacy/console code uses 0..5.
    // Convert the incoming weekday to the DB convention.
    if (weekdayZeroBased) {
        if (weekday >= 1 && weekday <= 6) return weekday - 1;
        return weekday;
    }

    // DB is 1-based.
    if (weekday >= 0 && weekday <= 5) return weekday + 1;
    return weekday;
}

int Database::normalizeWeekdayFromDb(int weekday)
{
    resolveWeekdayZeroBased();
    if (weekdayZeroBased) return weekday + 1;
    return weekday;
}

bool Database::getTeacherForScheduleId(int scheduleId, int& outTeacherId, std::string& outTeacherName)
{
    outTeacherId = 0;
    outTeacherName.clear();
    if (!db) return false;
    if (scheduleId <= 0) return true;

    const char* sql =
        "SELECT s.teacherid, COALESCE(u.name, '') "
        "FROM schedule s "
        "LEFT JOIN users u ON u.id = s.teacherid "
        "WHERE s.id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, scheduleId);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outTeacherId = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        outTeacherName = nameText ? reinterpret_cast<const char*>(nameText) : "";
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool Database::getUserIdByUsername(const std::string& username, int& outUserId)
{
    outUserId = 0;
    if (!db) return false;
    const char* sql = "SELECT id FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outUserId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

bool Database::getTeacherSubjectIds(int teacherId, std::vector<int>& outSubjectIds)
{
    outSubjectIds.clear();
    if (!db) return false;
    if (teacherId <= 0) return false;

    const char* sql = "SELECT subjectid FROM teachersubjects WHERE teacherid = ? ORDER BY subjectid;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, teacherId);

    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        outSubjectIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::setTeacherSubjects(int teacherId, const std::vector<int>& subjectIds)
{
    if (!db) return false;
    if (teacherId <= 0) return false;

    char* err = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return false;
    }

    sqlite3_stmt* delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM teachersubjects WHERE teacherid = ?;", -1, &delStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(delStmt, 1, teacherId);
    if (sqlite3_step(delStmt) != SQLITE_DONE) {
        sqlite3_finalize(delStmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(delStmt);

    sqlite3_stmt* insStmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO teachersubjects(teacherid, subjectid) VALUES (?, ?);", -1, &insStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    for (int sid : subjectIds) {
        if (sid <= 0) continue;
        sqlite3_reset(insStmt);
        sqlite3_clear_bindings(insStmt);
        sqlite3_bind_int(insStmt, 1, teacherId);
        sqlite3_bind_int(insStmt, 2, sid);
        if (sqlite3_step(insStmt) != SQLITE_DONE) {
            sqlite3_finalize(insStmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }
    sqlite3_finalize(insStmt);

    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    if (err) sqlite3_free(err);
    return true;
}

bool Database::getTeacherGroupIds(int teacherId, std::vector<int>& outGroupIds)
{
    outGroupIds.clear();
    if (!db) return false;
    if (teacherId <= 0) return false;

    const char* sql = "SELECT groupid FROM teachergroups WHERE teacherid = ? ORDER BY groupid;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, teacherId);

    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        outGroupIds.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::setTeacherGroups(int teacherId, const std::vector<int>& groupIds)
{
    if (!db) return false;
    if (teacherId <= 0) return false;

    char* err = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return false;
    }

    sqlite3_stmt* delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM teachergroups WHERE teacherid = ?;", -1, &delStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(delStmt, 1, teacherId);
    if (sqlite3_step(delStmt) != SQLITE_DONE) {
        sqlite3_finalize(delStmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(delStmt);

    sqlite3_stmt* insStmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO teachergroups(teacherid, groupid) VALUES (?, ?);", -1, &insStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    for (int gid : groupIds) {
        if (gid < 0) continue;
        sqlite3_reset(insStmt);
        sqlite3_clear_bindings(insStmt);
        sqlite3_bind_int(insStmt, 1, teacherId);
        sqlite3_bind_int(insStmt, 2, gid);
        if (sqlite3_step(insStmt) != SQLITE_DONE) {
            sqlite3_finalize(insStmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }
    sqlite3_finalize(insStmt);

    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    if (err) sqlite3_free(err);
    return true;
}

bool Database::countScheduleEntriesForTeacher(int teacherId, int& outCount)
{
    outCount = 0;
    if (!db) return false;
    if (teacherId <= 0) return false;

    const char* sql = "SELECT COUNT(*) FROM schedule WHERE teacherid = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, teacherId);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outCount = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

bool Database::deleteTeacherWithDependencies(int teacherId)
{
    if (!db) return false;
    if (teacherId <= 0) return false;

    char* err = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return false;
    }

    auto rollback = [&]() {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    };

    sqlite3_stmt* stmt = nullptr;

    // 1) schedule
    if (sqlite3_prepare_v2(db, "DELETE FROM schedule WHERE teacherid = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_int(stmt, 1, teacherId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    // 2) teachersubjects
    if (sqlite3_prepare_v2(db, "DELETE FROM teachersubjects WHERE teacherid = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_int(stmt, 1, teacherId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    // 3) teachergroups
    if (sqlite3_prepare_v2(db, "DELETE FROM teachergroups WHERE teacherid = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_int(stmt, 1, teacherId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    // 4) user
    if (sqlite3_prepare_v2(db, "DELETE FROM users WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_int(stmt, 1, teacherId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        rollback();
        return false;
    }
    if (err) sqlite3_free(err);
    return true;
}

bool Database::getGroupsFromScheduleForTeacher(int teacherId,
                                              std::vector<std::pair<int, std::string>>& outGroups)
{
    outGroups.clear();
    if (!db) return false;
    if (teacherId <= 0) return false;

    const char* sql = R"SQL(
        SELECT DISTINCT g.id, g.name
        FROM schedule s
        JOIN groups g ON s.groupid = g.id
        WHERE s.teacherid = ?
          AND s.groupid <> 0
        ORDER BY g.id
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, teacherId);

    int rc = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outGroups.emplace_back(id, name);
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ===== Constructor & Destructor =====

Database::Database(const std::string& file)
    : db(nullptr), fileName(file) {}

Database::~Database() {
    disconnect();
}

// ===== Connection Management =====

bool Database::connect() {
    if (db != nullptr) {
        return true;  // Already connected
    }

    int rc = sqlite3_open(fileName.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Error opening SQLite DB: "
                  << (sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "unknown")
                  << std::endl;
        db = nullptr;
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    std::cout << "[✓] SQLite DB opened: " << fileName << std::endl;
    return true;
}

bool Database::isConnected() const {
    return db != nullptr;
}

void Database::disconnect() {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
        std::cout << "[✓] SQLite connection closed." << std::endl;
    }
}

// ===== Execute SQL =====

bool Database::execute(const std::string& sql) {
    if (!isConnected()) {
        std::cerr << "[✗] Cannot execute query: DB not open." << std::endl;
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] SQLite Error: "
                  << (errMsg ? errMsg : "unknown")
                  << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

// ===== Initialize Schema with Auto-Migration =====
bool Database::initialize()
{
    if (!db) {
        std::cerr << "[✗] initialize: БД не открыта\n";
        return false;
    }

    const char* sql =
        // groups
        "CREATE TABLE IF NOT EXISTS groups ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT    NOT NULL UNIQUE"
        ");"

        // semesters
        "CREATE TABLE IF NOT EXISTS semesters ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name       TEXT NOT NULL UNIQUE,"
        "  startdate TEXT,"
        "  enddate   TEXT"
        ");"

        // users
        "CREATE TABLE IF NOT EXISTS users ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username  TEXT NOT NULL UNIQUE,"
        "  password  TEXT NOT NULL,"
        "  role      TEXT NOT NULL CHECK (role IN ('admin','teacher','student')),"
        "  name      TEXT NOT NULL,"
        "  groupid  INTEGER,"
        "  subgroup INTEGER DEFAULT 0,"
        "  FOREIGN KEY (groupid) REFERENCES groups(id)"
        ");"

        // subjects
        "CREATE TABLE IF NOT EXISTS subjects ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE"
        ");"

        // teachergroupjects
        "CREATE TABLE IF NOT EXISTS teachersubjects ("
        "  teacherid INTEGER NOT NULL,"
        "  subjectid INTEGER NOT NULL,"
        "  PRIMARY KEY (teacherid, subjectid),"
        "  FOREIGN KEY (teacherid) REFERENCES users(id),"
        "  FOREIGN KEY (subjectid) REFERENCES subjects(id)"
        ");"

        // teachergroups
        "CREATE TABLE IF NOT EXISTS teachergroups ("
        "  teacherid INTEGER NOT NULL,"
        "  groupid   INTEGER NOT NULL,"
        "  PRIMARY KEY (teacherid, groupid),"
        "  FOREIGN KEY (teacherid) REFERENCES users(id),"
        "  FOREIGN KEY (groupid)   REFERENCES groups(id)"
        ");"

        // schedule
        "CREATE TABLE IF NOT EXISTS schedule ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  groupid      INTEGER NOT NULL,"
        "  subgroup     INTEGER NOT NULL,"
        "  weekday       INTEGER NOT NULL,"
        "  lessonnumber INTEGER NOT NULL,"
        "  weekofcycle INTEGER NOT NULL CHECK(weekofcycle BETWEEN 1 AND 4),"
        "  subjectid    INTEGER NOT NULL,"
        "  teacherid    INTEGER NOT NULL,"
        "  room          TEXT,"
        "  lessontype   TEXT,"
        "  FOREIGN KEY (groupid)   REFERENCES groups(id),"
        "  FOREIGN KEY (subjectid) REFERENCES subjects(id),"
        "  FOREIGN KEY (teacherid) REFERENCES users(id)"
        ");"

        "CREATE INDEX IF NOT EXISTS idxschedulegroupweek "
        "ON schedule(groupid, weekday, weekofcycle);"
        "CREATE INDEX IF NOT EXISTS idxscheduleteacher "
        "ON schedule(teacherid);"
        "CREATE INDEX IF NOT EXISTS idxscheduleroom "
        "ON schedule(room);"

        // grades
        "CREATE TABLE IF NOT EXISTS grades ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  studentid  INTEGER NOT NULL,"
        "  subjectid  INTEGER NOT NULL,"
        "  semesterid INTEGER NOT NULL,"
        "  value       INTEGER NOT NULL CHECK(value BETWEEN 0 AND 10),"
        "  date        TEXT,"
        "  gradetype  TEXT,"
        "  FOREIGN KEY (studentid)  REFERENCES users(id),"
        "  FOREIGN KEY (subjectid)  REFERENCES subjects(id),"
        "  FOREIGN KEY (semesterid) REFERENCES semesters(id)"
        ");"

        // gradechanges
        "CREATE TABLE IF NOT EXISTS gradechanges ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  gradeid    INTEGER NOT NULL,"
        "  oldvalue    INTEGER,"
        "  newvalue    INTEGER,"
        "  changedby   INTEGER NOT NULL,"
        "  changedate TEXT NOT NULL,"
        "  comment     TEXT,"
        "  FOREIGN KEY (gradeid)   REFERENCES grades(id),"
        "  FOREIGN KEY (changedby) REFERENCES users(id)"
        ");"

        // cycleweeks
        "CREATE TABLE IF NOT EXISTS cycleweeks ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  weekofcycle INTEGER NOT NULL CHECK(weekofcycle BETWEEN 1 AND 4),"
        "  startdate    TEXT NOT NULL,"
        "  enddate      TEXT NOT NULL"
        ");"

        // absences
        "CREATE TABLE IF NOT EXISTS absences ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  studentid  INTEGER NOT NULL,"
        "  subjectid  INTEGER NOT NULL,"
        "  semesterid INTEGER NOT NULL,"
        "  hours       INTEGER NOT NULL,"
        "  date        TEXT,"
        "  type        TEXT,"
        "  FOREIGN KEY (studentid)  REFERENCES users(id),"
        "  FOREIGN KEY (subjectid)  REFERENCES subjects(id),"
        "  FOREIGN KEY (semesterid) REFERENCES semesters(id)"
        ");";

    char* errMsg = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SQLite] " << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    std::cout << "[✓] Структура БД инициализирована.\n";
    return true;
}

bool Database::initializeDemoData()
{
    if (!isConnected()) {
        std::cerr << "[✗] initializeDemoData: БД не подключена\n";
        return false;
    }

    const char* sql =
        "BEGIN TRANSACTION;"
        // сначала чистим зависимые таблицы
        "DELETE FROM grades;"
        "DELETE FROM gradechanges;"
        "DELETE FROM absences;"
        "DELETE FROM schedule;"
        "DELETE FROM teachersubjects;"
        "DELETE FROM teachergroups;"
        // потом основных пользователей, предметы, группы, недели
        "DELETE FROM users;"
        "DELETE FROM subjects;"
        "DELETE FROM semesters;"
        "DELETE FROM groups;"
        "DELETE FROM cycleweeks;"
        // сбрасываем AUTOINCREMENT
        "DELETE FROM sqlite_sequence WHERE name IN ("
        " 'users','subjects','groups','schedule',"
        " 'grades','gradechanges','absences','cycleweeks'"
        ");"

        // ===== НЕДЕЛИ ЦИКЛА =====
        "INSERT INTO cycleweeks (id, weekofcycle, startdate, enddate) VALUES"
        " (1, 1, '2025-09-01', '2025-09-07'),"
        " (2, 2, '2025-09-08', '2025-09-14'),"
        " (3, 3, '2025-09-15', '2025-09-21'),"
        " (4, 4, '2025-09-22', '2025-09-28'),"
        " (5, 1, '2025-09-29', '2025-10-05'),"
        " (6, 2, '2025-10-06', '2025-10-12'),"
        " (7, 3, '2025-10-13', '2025-10-19'),"
        " (8, 4, '2025-10-20', '2025-10-26'),"
        " (9, 1, '2025-10-27', '2025-11-02'),"
        " (10, 2, '2025-11-03', '2025-11-09'),"
        " (11, 3, '2025-11-10', '2025-11-16'),"
        " (12, 4, '2025-11-17', '2025-11-23'),"
        " (13, 1, '2025-11-24', '2025-11-30'),"
        " (14, 2, '2025-12-01', '2025-12-07'),"
        " (15, 3, '2025-12-08', '2025-12-14'),"
        " (16, 4, '2025-12-15', '2025-12-21'),"
        " (17, 1, '2025-12-22', '2025-12-27');"

        // ===== ГРУППЫ =====
        "INSERT INTO groups (id, name) VALUES "
        " (0, 'Общая лекция'),"
        " (1, '420601'),"
        " (2, '420602'),"
        " (3, '420603'),"
        " (4, '420604');"

        // ===== СТУДЕНТЫ 420601 =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('s420601_01', '123', 'student', 'Альбеков Кирилл Александрович', 1, 1),"
        " ('s420601_02', '123', 'student', 'Боричевский Алексей Владимирович', 1, 1),"
        " ('s420601_03', '123', 'student', 'Вильтовская Анастасия Дмитриевна', 1, 1),"
        " ('s420601_04', '123', 'student', 'Гвоздовский Максим Александрович', 1, 1),"
        " ('s420601_05', '123', 'student', 'Дикун Павел Дмитриевич', 1, 1),"
        " ('s420601_06', '123', 'student', 'Дудкевич Вера Витальевна', 1, 1),"
        " ('s420601_07', '123', 'student', 'Евдокимчик Дарья Андреевна', 1, 1),"
        " ('s420601_08', '123', 'student', 'Жулаев Даниил Александрович', 1, 1),"
        " ('s420601_09', '123', 'student', 'Кирей Владислав Олегович', 1, 1),"
        " ('s420601_10', '123', 'student', 'Концевич Кирилл Петрович', 1, 1),"
        " ('s420601_11', '123', 'student', 'Кузака Дмитрий Александрович', 1, 1),"
        " ('s420601_12', '123', 'student', 'Куликовский Степан Юрьевич', 1, 1),"
        " ('s420601_13', '123', 'student', 'Лях Елизавета Сергеевна', 1, 1),"
        " ('s420601_14', '123', 'student', 'Мавчун Артём Вадимович', 1, 2),"
        " ('s420601_15', '123', 'student', 'Макеенко Даниил Юрьевич', 1, 2),"
        " ('s420601_16', '123', 'student', 'Мартинович Максим Михайлович', 1, 2),"
        " ('s420601_17', '123', 'student', 'Мойсейчик Владислав Александрович', 1, 2),"
        " ('s420601_18', '123', 'student', 'Новик Павел Валерьевич', 1, 2),"
        " ('s420601_19', '123', 'student', 'Павлович Иван Владимирович', 1, 2),"
        " ('s420601_20', '123', 'student', 'Пейганович Кирилл Александрович', 1, 2),"
        " ('s420601_21', '123', 'student', 'Романченко Кристина Александровна', 1, 2),"
        " ('s420601_22', '123', 'student', 'Симонова Яна Владимировна', 1, 2),"
        " ('s420601_23', '123', 'student', 'Снитко Роман Евгеньевич', 1, 2),"
        " ('s420601_24', '123', 'student', 'Толстой Никита Игоревич', 1, 2),"
        " ('s420601_25', '123', 'student', 'Швайко Анастасия Витальевна', 1, 2),"
        " ('s420601_26', '123', 'student', 'Юдин Никита Сергеевич', 1, 2),"
        " ('s420601_27', '123', 'student', 'Янченко Артём Дмитриевич', 1, 2);"

        // ===== СТУДЕНТЫ 420602 =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('s420602_01', '123', 'student', 'Базылев Тимофей Андреевич', 2, 1),"
        " ('s420602_02', '123', 'student', 'Букач Павел Георгиевич', 2, 1),"
        " ('s420602_03', '123', 'student', 'Ватанабэ Фрэдо Такэхирович', 2, 1),"
        " ('s420602_04', '123', 'student', 'Вашкевич Полина Александровна', 2, 1),"
        " ('s420602_05', '123', 'student', 'Галкин Владислав Александрович', 2, 1),"
        " ('s420602_06', '123', 'student', 'Грабовский Алексей Кириллович', 2, 1),"
        " ('s420602_07', '123', 'student', 'Данько Павел Сергеевич', 2, 1),"
        " ('s420602_08', '123', 'student', 'Дранкевич Даниил Игоревич', 2, 1),"
        " ('s420602_09', '123', 'student', 'Дулин Родион Дмитриевич', 2, 1),"
        " ('s420602_10', '123', 'student', 'Каленько Алексей Александрович', 2, 1),"
        " ('s420602_11', '123', 'student', 'Кузьменков Иван Алексеевич', 2, 1),"
        " ('s420602_12', '123', 'student', 'Купревич Мария Михайловна', 2, 1),"
        " ('s420602_13', '123', 'student', 'Леончук Егор Дмитриевич', 2, 1),"
        " ('s420602_14', '123', 'student', 'Михаленя Максим Владимирович', 2, 1),"
        " ('s420602_15', '123', 'student', 'Мусохранова Екатерина Олеговна', 2, 1),"
        " ('s420602_16', '123', 'student', 'Нгуен Андрей Алексеевич', 2, 2),"
        " ('s420602_17', '123', 'student', 'Одинцов Георгий Юрьевич', 2, 2),"
        " ('s420602_18', '123', 'student', 'Павлов Евгений Александрович', 2, 2),"
        " ('s420602_19', '123', 'student', 'Родюков Глеб Олегович', 2, 2),"
        " ('s420602_20', '123', 'student', 'Рудоман Кристина Алексеевна', 2, 2),"
        " ('s420602_21', '123', 'student', 'Садовский Иван Витальевич', 2, 2),"
        " ('s420602_22', '123', 'student', 'Сазанович Максим Игоревич', 2, 2),"
        " ('s420602_23', '123', 'student', 'Смирнов Артём Алексеевич', 2, 2),"
        " ('s420602_24', '123', 'student', 'Толкачёв Никита Сергеевич', 2, 2),"
        " ('s420602_25', '123', 'student', 'Фурс Иван Александрович', 2, 2),"
        " ('s420602_26', '123', 'student', 'Черняков Александр Сергеевич', 2, 2),"
        " ('s420602_27', '123', 'student', 'Шкарубо Дмитрий Алексеевич', 2, 2),"
        " ('s420602_28', '123', 'student', 'Шостак Кирилл Александрович', 2, 2),"
        " ('s420602_29', '123', 'student', 'Язвинская Виктория Александровна', 2, 2),"
        " ('s420602_30', '123', 'student', 'Яцков Андрей Андреевич', 2, 2);"

        // ===== СТУДЕНТЫ 420603 =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('s420603_01', '123', 'student', 'Борутенко Богдан Владимирович', 3, 1),"
        " ('s420603_02', '123', 'student', 'Бугай Елизавета Андреевна', 3, 1),"
        " ('s420603_03', '123', 'student', 'Василевский Владислав Валерьевич', 3, 1),"
        " ('s420603_04', '123', 'student', 'Гарифуллин Арсений Альфредович', 3, 1),"
        " ('s420603_05', '123', 'student', 'Горбель Алексей Артёмович', 3, 1),"
        " ('s420603_06', '123', 'student', 'Горох Ульяна Руслановна', 3, 1),"
        " ('s420603_07', '123', 'student', 'Губко Денис Андреевич', 3, 1),"
        " ('s420603_08', '123', 'student', 'Дыдо Александра Владимировна', 3, 1),"
        " ('s420603_09', '123', 'student', 'Займист Арсений Александрович', 3, 1),"
        " ('s420603_10', '123', 'student', 'Ивашко Дарья Сергеевна', 3, 1),"
        " ('s420603_11', '123', 'student', 'Игнатчик Алексей Владимирович', 3, 1),"
        " ('s420603_12', '123', 'student', 'Кириленко Ольга Витальевна', 3, 1),"
        " ('s420603_13', '123', 'student', 'Козловский Даниил Сергеевич', 3, 1),"
        " ('s420603_14', '123', 'student', 'Корзун Владислав Владимирович', 3, 1),"
        " ('s420603_15', '123', 'student', 'Корниевич Вячеслав Викторович', 3, 2),"
        " ('s420603_16', '123', 'student', 'Куликовский Кирилл Дмитриевич', 3, 2),"
        " ('s420603_17', '123', 'student', 'Кучук Роман Владиславович', 3, 2),"
        " ('s420603_18', '123', 'student', 'Матусевич Вероника Климентьевна', 3, 2),"
        " ('s420603_19', '123', 'student', 'Микша Вячеслав Андреевич', 3, 2),"
        " ('s420603_20', '123', 'student', 'Новик Дарья Олеговна', 3, 2),"
        " ('s420603_21', '123', 'student', 'Песецкий Эдгар Иванович', 3, 2),"
        " ('s420603_22', '123', 'student', 'Петров Юрий Александрович', 3, 2),"
        " ('s420603_23', '123', 'student', 'Пущик Евгения Андреевна', 3, 2),"
        " ('s420603_24', '123', 'student', 'Рассоха Станислав Андреевич', 3, 2),"
        " ('s420603_25', '123', 'student', 'Рында Роман Дмитриевич', 3, 2),"
        " ('s420603_26', '123', 'student', 'Тарнапович Алексей Александрович', 3, 2),"
        " ('s420603_27', '123', 'student', 'Трояновский Александр Евгеньевич', 3, 2),"
        " ('s420603_28', '123', 'student', 'Чечулин Кирилл Алексеевич', 3, 2),"
        " ('s420603_29', '123', 'student', 'Шупеник Матвей Юрьевич', 3, 2);"

        // ===== СТУДЕНТЫ 420604 =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('s420604_01', '123', 'student', 'Александренков Денис Алексеевич', 4, 1),"
        " ('s420604_02', '123', 'student', 'Амельченко Роман Игоревич', 4, 1),"
        " ('s420604_03', '123', 'student', 'Борисюк Илья Александрович', 4, 1),"
        " ('s420604_04', '123', 'student', 'Винников Иван Александрович', 4, 1),"
        " ('s420604_05', '123', 'student', 'Гайдукевич Ксения Андреевна', 4, 1),"
        " ('s420604_06', '123', 'student', 'Гузова Александрова Евгеньевна', 4, 1),"
        " ('s420604_07', '123', 'student', 'Гуренков Данила Станиславович', 4, 1),"
        " ('s420604_08', '123', 'student', 'Драпеза Вероника Алексеевна', 4, 1),"
        " ('s420604_09', '123', 'student', 'Иосько Михаил Алексеевич', 4, 1),"
        " ('s420604_10', '123', 'student', 'Кацубо Андрей Евгеньевич', 4, 1),"
        " ('s420604_11', '123', 'student', 'Кирилушкин Андрей Романович', 4, 1),"
        " ('s420604_12', '123', 'student', 'Козлюк Павел Николаевич', 4, 1),"
        " ('s420604_13', '123', 'student', 'Колесник Владислав Александрович', 4, 1),"
        " ('s420604_14', '123', 'student', 'Королев Илья Александрович', 4, 1),"
        " ('s420604_15', '123', 'student', 'Куновский Дмитрий Сергеевич', 4, 1),"
        " ('s420604_16', '123', 'student', 'Леоновец Дана Андреевна', 4, 2),"
        " ('s420604_17', '123', 'student', 'Мамедов Рустам Джамилович', 4, 2),"
        " ('s420604_18', '123', 'student', 'Марчук Кирилл Геннадьевич', 4, 2),"
        " ('s420604_19', '123', 'student', 'Матусевич Егор Николаевич', 4, 2),"
        " ('s420604_20', '123', 'student', 'Мидляр Илья Алексеевич', 4, 2),"
        " ('s420604_21', '123', 'student', 'Могилевчик Тимофей Михайлович', 4, 2),"
        " ('s420604_22', '123', 'student', 'Попов Андрей Алексеевич', 4, 2),"
        " ('s420604_23', '123', 'student', 'Рогаль Алексей Сергеевич', 4, 2),"
        " ('s420604_24', '123', 'student', 'Савин Тимофей Александрович', 4, 2),"
        " ('s420604_25', '123', 'student', 'Савостикова Екатерина Дмитриевна', 4, 2),"
        " ('s420604_26', '123', 'student', 'Третьяк Арсений Валерьевич', 4, 2),"
        " ('s420604_27', '123', 'student', 'Тузова Виктория Сергеевна', 4, 2),"
        " ('s420604_28', '123', 'student', 'Хмара Матвей Сергеевич', 4, 2),"
        " ('s420604_29', '123', 'student', 'Шпаковская Василиса Сергеевна', 4, 2),"
        " ('s420604_30', '123', 'student', 'Янушковский Алексей Андрианович', 4, 2);"

        // ===== АДМИН =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('admin', '123', 'admin', 'Administrator', NULL, 0);"

        // ===== ПРЕПОДАВАТЕЛИ =====
        "INSERT INTO users (username, password, role, name, groupid, subgroup) VALUES "
        " ('t_batin',         '123', 'teacher', 'Батин Н. В.',         NULL, 0),"
        " ('t_bobrova',       '123', 'teacher', 'Боброва Т. С.',       NULL, 0),"
        " ('t_shilin',        '123', 'teacher', 'Шилин Л. Ю.',         NULL, 0),"
        " ('t_nehaychik',     '123', 'teacher', 'Нехайчик Е. В.',      NULL, 0),"
        " ('t_sevurnev',      '123', 'teacher', 'Севернёв А. М.',      NULL, 0),"
        " ('t_sluzhalik',     '123', 'teacher', 'Служалик В. Ю.',      NULL, 0),"
        " ('t_deriabina',     '123', 'teacher', 'Дерябина М. Ю.',      NULL, 0),"
        " ('t_gurevich',      '123', 'teacher', 'Гуревич О. В.',       NULL, 0),"
        " ('t_sharonova',     '123', 'teacher', 'Шаронова Е. И.',      NULL, 0),"
        " ('t_ryshkel',       '123', 'teacher', 'Рышкель О. С.',       NULL, 0),"
        " ('t_tsavlovskaya',  '123', 'teacher', 'Цявловская Н. В.',    NULL, 0),"
        " ('t_prigara',       '123', 'teacher', 'Пригара В. Н.',       NULL, 0),"
        " ('t_yuchkov',       '123', 'teacher', 'Ючков А. К.',         NULL, 0),"
        " ('t_german',        '123', 'teacher', 'Герман О. В.',        NULL, 0),"
        " ('t_puhir',         '123', 'teacher', 'Пухир Г. А.',         NULL, 0),"
        " ('t_krishchenovich','123','teacher','Крищенович В. А.',      NULL, 0),"
        " ('t_lappo',         '123', 'teacher', 'Лаппо А. И.',         NULL, 0),"
        " ('t_nehlebova',     '123', 'teacher', 'Нехлебова О. Ю.',     NULL, 0),"
        " ('t_ezovit',        '123', 'teacher', 'Езовит А. В.',        NULL, 0),"
        " ('t_trofimovich',   '123', 'teacher', 'Трофимович А. Ф.',    NULL, 0),"
        " ('t_melnik',        '123', 'teacher', 'Мельник А. А.',       NULL, 0),"
        " ('t_phys',          '123', 'teacher', 'Физкультуров Ф. Ф.',  NULL, 0);"

        // ===== ПРЕДМЕТЫ =====
        "INSERT INTO subjects (id, name) VALUES "
        " (1, 'АПЭЦ'),"
        " (2, 'ТВиМС'),"
        " (3, 'ФизК'),"
        " (4, 'ТГ'),"
        " (5, 'ВМиКА'),"
        " (6, 'БЖЧ'),"
        " (7, 'ООП'),"
        " (8, 'ОИнфБ'),"
        " (9, 'БД'),"
        " (10, 'МСиСвИТ'),"
        " (11, 'Инф. час'),"
        " (12, 'К.Ч.');"

        // teachergroupjects (кто какой предмет ведёт)
        "INSERT INTO teachersubjects (teacherid, subjectid) VALUES "
        "(120, 1),"
        "(129, 1),"
        "(121, 1),"
        "(139, 3),"
        "(122, 4),"
        "(123, 4),"
        "(118, 5),"
        "(119, 5),"
        "(127, 6),"
        "(128, 6),"
        "(131, 7),"
        "(130, 7),"
        "(132, 8),"
        "(133, 8),"
        "(134, 9),"
        "(135, 9),"
        "(124, 10),"
        "(136, 11),"
        "(136, 12),"
        "(130, 11),"
        "(130, 12),"
        "(137, 11),"
        "(137, 12),"
        "(138, 11),"
        "(138, 12);"

        // ===== СЕМЕСТРЫ =====
        "INSERT INTO semesters (id, name, startdate, enddate) VALUES "
        " (1, '1 семестр 2025/2026', '2025-09-01', '2025-12-31');"
        ;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка заполнения демо-данными: "
                  << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    // дальше — твоя логика teachergroups через prepared statements + verify + COMMIT
    // (она у тебя правильная, её надо вставить целиком без изменений, просто заменить db_ -> db)
    {
        const std::vector<std::string> allGroups = {"420601", "420602", "420603", "420604"};
        const std::vector<std::pair<std::string, std::vector<std::string>>> teacherGroups = {
            {"t_shilin", allGroups},
            {"t_prigara", {"420602"}},
            {"t_nehaychik", {"420603", "420604"}},
            {"t_phys", allGroups},
            {"t_sevurnev", allGroups},
            {"t_sluzhalik", allGroups},
            {"t_batin", allGroups},
            {"t_bobrova", allGroups},
            {"t_ryshkel", allGroups},
            {"t_tsavlovskaya", {"420603", "420604"}},
            {"t_german", allGroups},
            {"t_yuchkov", allGroups},
            {"t_puhir", allGroups},
            {"t_krishchenovich", allGroups},
            {"t_lappo", allGroups},
            {"t_nehlebova", {"420603", "420604"}},
            {"t_deriabina", allGroups},
            {"t_ezovit", {"420601"}},
            {"t_trofimovich", {"420603"}},
            {"t_melnik", {"420604"}},
            {"t_gurevich", allGroups},
            {"t_sharonova", allGroups},
        };

        const char* insertSql =
            "INSERT OR IGNORE INTO teachergroups(teacherid, groupid) VALUES (?, ?);";

        sqlite3_stmt* insertStmt = nullptr;
        if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка подготовки INSERT teachergroups: "
                      << (sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "unknown") << "\n";
            return false;
        }

        for (const auto& tg : teacherGroups) {
            const std::string& username = tg.first;
            int teacherId = 0;
            if (!getUserIdByUsername(username, teacherId) || teacherId <= 0) {
                sqlite3_finalize(insertStmt);
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                std::cerr << "[✗] teachergroups: не найден пользователь teacher username='"
                          << username << "'\n";
                return false;
            }

            for (const auto& groupName : tg.second) {
                const int groupId = getGroupIdByName(db, groupName);
                if (groupId <= 0) {
                    sqlite3_finalize(insertStmt);
                    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                    std::cerr << "[✗] teachergroups: не найдена группа name='"
                              << groupName << "'\n";
                    return false;
                }

                sqlite3_reset(insertStmt);
                sqlite3_clear_bindings(insertStmt);
                sqlite3_bind_int(insertStmt, 1, teacherId);
                sqlite3_bind_int(insertStmt, 2, groupId);

                const int stepRc = sqlite3_step(insertStmt);
                if (stepRc != SQLITE_DONE) {
                    sqlite3_finalize(insertStmt);
                    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                    std::cerr << "[✗] teachergroups: ошибка INSERT для teacher='"
                              << username << "', group='" << groupName << "': "
                              << (sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "unknown") << "\n";
                    return false;
                }
            }
        }

        sqlite3_finalize(insertStmt);
    }

    {
        const char* verifySql =
            "SELECT u.username, u.name, GROUP_CONCAT(g.name, ', ') "
            "FROM teachergroups tg "
            "JOIN users u ON u.id = tg.teacherid "
            "JOIN groups g ON g.id = tg.groupid "
            "GROUP BY u.id "
            "ORDER BY u.username;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, verifySql, -1, &stmt, nullptr) != SQLITE_OK) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка подготовки SELECT teachergroups verify: "
                      << (sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "unknown") << "\n";
            return false;
        }

        std::cout << "\n[DEMO] teacher username -> groups\n";
        std::cout << std::left
                  << std::setw(18) << "username" << " | "
                  << std::setw(22) << "name" << " | "
                  << "groups" << "\n";
        std::cout << std::string(80, '-') << "\n";

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const unsigned char* usernameText = sqlite3_column_text(stmt, 0);
            const unsigned char* nameText = sqlite3_column_text(stmt, 1);
            const unsigned char* groupsText = sqlite3_column_text(stmt, 2);

            const std::string username = usernameText ? reinterpret_cast<const char*>(usernameText) : "";
            const std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
            const std::string groups = groupsText ? reinterpret_cast<const char*>(groupsText) : "";

            std::cout << std::left
                      << std::setw(18) << username << " | "
                      << std::setw(22) << name << " | "
                      << groups << "\n";
        }

        if (rc != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка выполнения SELECT teachergroups verify: "
                      << (sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "unknown") << "\n";
            return false;
        }

        sqlite3_finalize(stmt);
    }

    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка COMMIT демо-данных: "
                  << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    std::cout << "[✓] БД заполнена демо-данными для групп 420601–420604.\n";
    std::cout << "[✓] Студентов: 116, Преподавателей: 22, Админ: 1\n";
    return true;
}

// ===== Find User =====

bool Database::findUser(const std::string& username,
                        const std::string& password,
                        int& outId,
                        std::string& outName,
                        std::string& outRole) {
    if (!isConnected()) {
        return false;
    }

    const char* sql =
        "SELECT id, name, role "
        "FROM users "
        "WHERE username = ? AND password = ? "
        "LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outId = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const unsigned char* roleText = sqlite3_column_text(stmt, 2);
        outName = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outRole = roleText ? reinterpret_cast<const char*>(roleText) : "";
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

// ===== Get All Resources =====

bool Database::getAllSemesters(std::vector<std::pair<int, std::string>>& outSemesters) {
    outSemesters.clear();
    if (!db) {
        std::cerr << "[✗] getAllSemesters: DB not connected\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM semesters ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllSemesters: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSemesters.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllSemesters: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getSubjectIdByName(const std::string& subjectName, int& outSubjectId)
{
    outSubjectId = 0;
    if (!db) return false;
    if (subjectName.empty()) return true;

    const char* sql = "SELECT id FROM subjects WHERE name = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, subjectName.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outSubjectId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool Database::getStudentAverageGradeForMonth(int studentId, int semesterId,
                                              int year, int month, int subjectId,
                                              double& outAvg, int& outCount)
{
    outAvg = 0.0;
    outCount = 0;
    if (!db) return false;
    if (studentId <= 0 || semesterId <= 0 || year <= 0 || month < 1 || month > 12) return false;

    const char* sqlAll =
        "SELECT COALESCE(AVG(g.value), 0.0), COALESCE(COUNT(*), 0) "
        "FROM grades g "
        "WHERE g.studentid = ? AND g.semesterid = ? "
        "AND strftime('%Y', g.date) = ? "
        "AND strftime('%m', g.date) = ?;";

    const char* sqlSubj =
        "SELECT COALESCE(AVG(g.value), 0.0), COALESCE(COUNT(*), 0) "
        "FROM grades g "
        "WHERE g.studentid = ? AND g.semesterid = ? AND g.subjectid = ? "
        "AND strftime('%Y', g.date) = ? "
        "AND strftime('%m', g.date) = ?;";

    sqlite3_stmt* stmt = nullptr;
    const char* sql = (subjectId > 0) ? sqlSubj : sqlAll;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int idx = 1;
    sqlite3_bind_int(stmt, idx++, studentId);
    sqlite3_bind_int(stmt, idx++, semesterId);
    if (subjectId > 0) {
        sqlite3_bind_int(stmt, idx++, subjectId);
    }

    const std::string ys = std::to_string(year);
    char msBuf[3];
    std::snprintf(msBuf, sizeof(msBuf), "%02d", month);

    sqlite3_bind_text(stmt, idx++, ys.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, msBuf, -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outAvg = sqlite3_column_double(stmt, 0);
        outCount = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool Database::getStudentAbsenceHoursForMonth(int studentId, int semesterId,
                                              int year, int month, int subjectId,
                                              int& outTotalHours, int& outUnexcusedHours)
{
    outTotalHours = 0;
    outUnexcusedHours = 0;
    if (!db) return false;
    if (studentId <= 0 || semesterId <= 0 || year <= 0 || month < 1 || month > 12) return false;

    const char* sqlAll =
        "SELECT "
        "  COALESCE(SUM(a.hours), 0), "
        "  COALESCE(SUM(CASE WHEN COALESCE(a.type,'') <> 'excused' THEN a.hours ELSE 0 END), 0) "
        "FROM absences a "
        "WHERE a.studentid = ? AND a.semesterid = ? "
        "AND strftime('%Y', a.date) = ? "
        "AND strftime('%m', a.date) = ?;";

    const char* sqlSubj =
        "SELECT "
        "  COALESCE(SUM(a.hours), 0), "
        "  COALESCE(SUM(CASE WHEN COALESCE(a.type,'') <> 'excused' THEN a.hours ELSE 0 END), 0) "
        "FROM absences a "
        "WHERE a.studentid = ? AND a.semesterid = ? AND a.subjectid = ? "
        "AND strftime('%Y', a.date) = ? "
        "AND strftime('%m', a.date) = ?;";

    sqlite3_stmt* stmt = nullptr;
    const char* sql = (subjectId > 0) ? sqlSubj : sqlAll;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int idx = 1;
    sqlite3_bind_int(stmt, idx++, studentId);
    sqlite3_bind_int(stmt, idx++, semesterId);
    if (subjectId > 0) sqlite3_bind_int(stmt, idx++, subjectId);

    const std::string ys = std::to_string(year);
    char msBuf[3];
    std::snprintf(msBuf, sizeof(msBuf), "%02d", month);
    sqlite3_bind_text(stmt, idx++, ys.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, msBuf, -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outTotalHours = sqlite3_column_int(stmt, 0);
        outUnexcusedHours = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW || rc == SQLITE_DONE);
}

bool Database::updateUser(int userId,
                          const std::string& username,
                          const std::string& name,
                          const std::string& role,
                          int groupId,
                          int subgroup,
                          const std::string& passwordOrEmpty)
{
    if (!isConnected()) {
        std::cerr << "[✗] updateUser: DB not connected\n";
        return false;
    }
    if (userId <= 0) return false;

    bool withPassword = false;
    for (char ch : passwordOrEmpty) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            withPassword = true;
            break;
        }
    }
    const char* sqlNoPass = "UPDATE users SET username=?, name=?, role=?, groupid=?, subgroup=? WHERE id=?;";
    const char* sqlWithPass = "UPDATE users SET username=?, password=?, name=?, role=?, groupid=?, subgroup=? WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, withPassword ? sqlWithPass : sqlNoPass, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] updateUser prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    int idx = 1;
    sqlite3_bind_text(stmt, idx++, username.c_str(), -1, SQLITE_TRANSIENT);
    if (withPassword) {
        sqlite3_bind_text(stmt, idx++, passwordOrEmpty.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, idx++, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, role.c_str(), -1, SQLITE_TRANSIENT);

    if (groupId <= 0) sqlite3_bind_null(stmt, idx++);
    else sqlite3_bind_int(stmt, idx++, groupId);
    sqlite3_bind_int(stmt, idx++, subgroup);
    sqlite3_bind_int(stmt, idx++, userId);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::getSubjectsForTeacherInGroupSchedule(int teacherId, int groupId,
                                                    std::vector<std::pair<int, std::string>>& outSubjects)
{
    outSubjects.clear();
    if (!db) return false;

    const char* sql = R"SQL(
        SELECT DISTINCT s.id, s.name
        FROM schedule sch
        JOIN subjects s ON sch.subjectid = s.id
        WHERE sch.teacherid = ?
          AND (sch.groupid = ? OR sch.groupid = 0)
        ORDER BY s.name
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, groupId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSubjects.emplace_back(id, name);
    }

    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

bool Database::getScheduleForTeacherWeekWithRoom(
    int teacherId,
    int weekOfCycle,
    int studentSubgroup,
    std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string,std::string>>& outRows)
{
    outRows.clear();
    if (!isConnected()) return false;

    const char* sql = R"SQL(
        SELECT sch.id,
               sch.groupid,
               sch.weekday,
               sch.lessonnumber,
               sch.subgroup,
               subj.name,
               COALESCE(sch.room, ''),
               COALESCE(sch.lessontype, ''),
               COALESCE(gr.name, '')
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        LEFT JOIN groups gr ON sch.groupid = gr.id
        WHERE sch.teacherid = ?
          AND sch.weekofcycle = ?
          AND (? = 0 OR sch.subgroup = 0 OR sch.subgroup = ?)
        ORDER BY sch.weekday, sch.lessonnumber, sch.groupid, sch.subgroup
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, weekOfCycle);
    sqlite3_bind_int(stmt, 3, studentSubgroup);
    sqlite3_bind_int(stmt, 4, studentSubgroup);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int scheduleId = sqlite3_column_int(stmt, 0);
        const int groupId = sqlite3_column_int(stmt, 1);
        const int weekday = normalizeWeekdayFromDb(sqlite3_column_int(stmt, 2));
        const int lessonNumber = sqlite3_column_int(stmt, 3);
        const int subgroup = sqlite3_column_int(stmt, 4);

        const char* subjName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* room = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const char* ltype = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* groupName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        outRows.emplace_back(
            scheduleId,
            groupId,
            weekday,
            lessonNumber,
            subgroup,
            subjName ? subjName : "",
            room ? room : "",
            ltype ? ltype : "",
            groupName ? groupName : ""
        );
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getScheduleForTeacherGroupWeekWithRoom(
    int teacherId,
    int groupId,
    int weekOfCycle,
    int studentSubgroup,
    std::vector<std::tuple<int, int, int, int, int, std::string, std::string, std::string>>& outRows)
{
    outRows.clear();
    if (!isConnected()) return false;

    const char* sql = R"SQL(
        SELECT sch.id,
               sch.subjectid,
               sch.weekday,
               sch.lessonnumber,
               sch.subgroup,
               subj.name,
               COALESCE(sch.room, ''),
               COALESCE(sch.lessontype, '')
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        WHERE sch.teacherid = ?
          AND (sch.groupid = ? OR sch.groupid = 0)
          AND sch.weekofcycle = ?
          AND (? = 0 OR sch.subgroup = 0 OR sch.subgroup = ?)
        ORDER BY sch.weekday, sch.lessonnumber, sch.subgroup
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, groupId);
    sqlite3_bind_int(stmt, 3, weekOfCycle);
    sqlite3_bind_int(stmt, 4, studentSubgroup);
    sqlite3_bind_int(stmt, 5, studentSubgroup);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId = sqlite3_column_int(stmt, 0);
        int subjectId = sqlite3_column_int(stmt, 1);
        int weekday = normalizeWeekdayFromDb(sqlite3_column_int(stmt, 2));
        int lessonNumber = sqlite3_column_int(stmt, 3);
        int subgroup = sqlite3_column_int(stmt, 4);
        const char* subjName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* room = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const char* ltype = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        outRows.emplace_back(
            scheduleId,
            subjectId,
            weekday,
            lessonNumber,
            subgroup,
            subjName ? subjName : "",
            room ? room : "",
            ltype ? ltype : ""
        );
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllUsers(
    std::vector<std::tuple<int, std::string, std::string, std::string, int, int>>& outUsers
)
{
    outUsers.clear();
    if (!db) {
        std::cerr << "[✗] getAllUsers: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT id, username, name, role, COALESCE(groupid, 0), COALESCE(subgroup, 0) "
        "FROM users ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllUsers: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* uText = sqlite3_column_text(stmt, 1);
        const unsigned char* nText = sqlite3_column_text(stmt, 2);
        const unsigned char* rText = sqlite3_column_text(stmt, 3);
        const int groupId = sqlite3_column_int(stmt, 4);
        const int subgroup = sqlite3_column_int(stmt, 5);

        const std::string username = uText ? reinterpret_cast<const char*>(uText) : "";
        const std::string name = nText ? reinterpret_cast<const char*>(nText) : "";
        const std::string role = rText ? reinterpret_cast<const char*>(rText) : "";

        outUsers.emplace_back(id, username, name, role, groupId, subgroup);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllUsers: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllGroups(std::vector<std::pair<int, std::string>>& outGroups) {
    outGroups.clear();
    if (!db) {
        std::cerr << "[✗] getAllGroups: DB not connected\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM groups ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllGroups: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outGroups.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllGroups: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllSubjects(std::vector<std::pair<int, std::string>>& outSubjects) {
    outSubjects.clear();
    if (!db) {
        std::cerr << "[✗] getAllSubjects: DB not connected\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM subjects ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllSubjects: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSubjects.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllSubjects: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllTeachers(std::vector<std::pair<int, std::string>>& outTeachers) {
    outTeachers.clear();
    if (!db) {
        std::cerr << "[✗] getAllTeachers: DB not connected\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM users WHERE role = 'teacher' ORDER BY name;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllTeachers: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outTeachers.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllTeachers: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getSubjectsForTeacher(int teacherId,
                                      std::vector<std::pair<int, std::string>>& outSubjects) {
    outSubjects.clear();
    if (!db) {
        std::cerr << "[✗] getSubjectsForTeacher: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT s.id, s.name "
        "FROM teachersubjects ts "
        "JOIN subjects s ON ts.subjectid = s.id "
        "WHERE ts.teacherid = ? "
        "ORDER BY s.id;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getSubjectsForTeacher: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSubjects.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getSubjectsForTeacher: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// ===== Students & Groups =====

bool Database::getStudentsOfGroup(int groupId,
                                   std::vector<std::pair<int, std::string>>& outStudents) {
    outStudents.clear();
    if (!db) {
        std::cerr << "[✗] getStudentsOfGroup: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT id, name "
        "FROM users "
        "WHERE role = 'student' AND groupid = ? "
        "ORDER BY name;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentsOfGroup: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, groupId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outStudents.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentsOfGroup: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getStudentGroupAndSubgroup(int studentId,
                                          int& outGroupId,
                                          int& outSubgroup) {
    outGroupId = 0;
    outSubgroup = 0;
    if (!db) {
        std::cerr << "[✗] getStudentGroupAndSubgroup: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT COALESCE(groupid, 0), COALESCE(subgroup, 0) "
        "FROM users "
        "WHERE id = ? AND role = 'student' "
        "LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentGroupAndSubgroup: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outGroupId = sqlite3_column_int(stmt, 0);
        outSubgroup = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return true;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    } else {
        std::cerr << "[✗] getStudentGroupAndSubgroup: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
}

// ===== Grades =====

bool Database::getStudentGradesForSemester(int studentId,
                                            int semesterId,
                                            std::vector<std::tuple<std::string, int, std::string, std::string>>& outGrades) {
    outGrades.clear();
    if (!db) {
        std::cerr << "[✗] getStudentGradesForSemester: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT s.name, g.value, COALESCE(g.date, ''), COALESCE(g.gradetype, '') "
        "FROM grades g "
        "JOIN subjects s ON g.subjectid = s.id "
        "WHERE g.studentid = ? AND g.semesterid = ? "
        "ORDER BY s.name, g.date";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentGradesForSemester: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, semesterId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* subjText = sqlite3_column_text(stmt, 0);
        int value = sqlite3_column_int(stmt, 1);
        const unsigned char* dateText = sqlite3_column_text(stmt, 2);
        const unsigned char* typeText = sqlite3_column_text(stmt, 3);
        std::string subject = subjText ? reinterpret_cast<const char*>(subjText) : "";
        std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string gtype = typeText ? reinterpret_cast<const char*>(typeText) : "";
        outGrades.emplace_back(subject, value, date, gtype);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentGradesForSemester: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getStudentSubjectGrades(int studentId,
                                        int subjectId,
                                        int semesterId,
                                        std::vector<std::tuple<int, std::string, std::string>>& outGrades) {
    outGrades.clear();
    if (!db) {
        std::cerr << "[✗] getStudentSubjectGrades: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT value, COALESCE(date, ''), COALESCE(gradetype, '') "
        "FROM grades "
        "WHERE studentid = ? AND subjectid = ? AND semesterid = ? "
        "ORDER BY date";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentSubjectGrades: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int value = sqlite3_column_int(stmt, 0);
        const unsigned char* dateText = sqlite3_column_text(stmt, 1);
        const unsigned char* typeText = sqlite3_column_text(stmt, 2);
        std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string type = typeText ? reinterpret_cast<const char*>(typeText) : "";
        outGrades.emplace_back(value, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentSubjectGrades: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addGrade(int studentId, int subjectId, int semesterId,
                        int value, const std::string& date, const std::string& gradeType) {
    if (!db) {
        std::cerr << "[✗] addGrade: DB not connected\n";
        return false;
    }

    const char* sql =
        "INSERT INTO grades (studentid, subjectid, semesterid, value, date, gradetype) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] addGrade prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);
    sqlite3_bind_int(stmt, 4, value);
    sqlite3_bind_text(stmt, 5, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, gradeType.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::findGradeId(int studentId, int subjectId, int semesterId,
                           const std::string& date, int& outGradeId)
{
    outGradeId = 0;
    if (!db) return false;

    const char* sql =
        "SELECT id FROM grades WHERE studentid = ? AND subjectid = ? AND semesterid = ? AND date = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outGradeId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return true;
}

bool Database::upsertGradeByKey(int studentId, int subjectId, int semesterId,
                                int value, const std::string& date, const std::string& gradeType)
{
    if (!db) return false;

    int gradeId = 0;
    if (!findGradeId(studentId, subjectId, semesterId, date, gradeId)) return false;

    if (gradeId > 0) {
        return updateGrade(gradeId, value, date, gradeType);
    }
    return addGrade(studentId, subjectId, semesterId, value, date, gradeType);
}

bool Database::getGradeById(int gradeId, int& outValue, std::string& outDate, std::string& outGradeType)
{
    outValue = 0;
    outDate.clear();
    outGradeType.clear();
    if (!db) return false;

    const char* sql =
        "SELECT value, COALESCE(date, ''), COALESCE(gradetype, '') FROM grades WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, gradeId);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outValue = sqlite3_column_int(stmt, 0);
        const unsigned char* dateText = sqlite3_column_text(stmt, 1);
        const unsigned char* typeText = sqlite3_column_text(stmt, 2);
        outDate = dateText ? reinterpret_cast<const char*>(dateText) : "";
        outGradeType = typeText ? reinterpret_cast<const char*>(typeText) : "";
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

bool Database::deleteGrade(int gradeId) {
    if (!db) {
        std::cerr << "[✗] deleteGrade: DB not connected\n";
        return false;
    }

    const char* sql = "DELETE FROM grades WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] deleteGrade prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, gradeId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        return true;
    }

    return false;
}

// ===== Absences =====

bool Database::getStudentUnexcusedAbsences(int studentId,
                                            int semesterId,
                                            int& outUnexcusedHours) {
    outUnexcusedHours = 0;
    if (!db) return false;

    const char* sql =
        "SELECT SUM(hours) FROM absences "
        "WHERE studentid = ? AND semesterid = ? AND type = 'unexcused';";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, semesterId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outUnexcusedHours = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::deleteTodayAbsence(int studentId,
                                   int subjectId,
                                   int semesterId,
                                   const std::string& date) {
    if (!db) return false;

    const char* sql =
        "DELETE FROM absences "
        "WHERE studentid = ? AND subjectid = ? AND semesterid = ? AND date = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return changes > 0;
}

bool Database::findAbsenceId(int studentId, int subjectId, int semesterId,
                             const std::string& date, int& outAbsenceId)
{
    outAbsenceId = 0;
    if (!db) return false;

    const char* sql =
        "SELECT id FROM absences WHERE studentid = ? AND subjectid = ? AND semesterid = ? AND date = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outAbsenceId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return true;
}

bool Database::upsertAbsenceByKey(int studentId, int subjectId, int semesterId,
                                  int hours, const std::string& date, const std::string& type)
{
    if (!db) return false;

    int absenceId = 0;
    if (!findAbsenceId(studentId, subjectId, semesterId, date, absenceId)) return false;

    if (absenceId > 0) {
        const char* sql = "UPDATE absences SET hours = ?, type = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, hours);
        sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, absenceId);
        const int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    return addAbsence(studentId, subjectId, semesterId, hours, date, type);
}

bool Database::getAbsenceById(int absenceId, int& outHours, std::string& outDate, std::string& outType)
{
    outHours = 0;
    outDate.clear();
    outType.clear();
    if (!db) return false;

    const char* sql =
        "SELECT hours, COALESCE(date, ''), COALESCE(type, '') FROM absences WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, absenceId);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outHours = sqlite3_column_int(stmt, 0);
        const unsigned char* dateText = sqlite3_column_text(stmt, 1);
        const unsigned char* typeText = sqlite3_column_text(stmt, 2);
        outDate = dateText ? reinterpret_cast<const char*>(dateText) : "";
        outType = typeText ? reinterpret_cast<const char*>(typeText) : "";
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

bool Database::deleteAbsence(int absenceId)
{
    if (!db) return false;

    const char* sql = "DELETE FROM absences WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, absenceId);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}

bool Database::getGroupSubjectAbsencesSummary(
    int groupId,
    int subjectId,
    int semesterId,
    std::vector<std::tuple<int, std::string, int>>& outRows) {
    outRows.clear();
    if (!db) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT u.id, u.name, COALESCE(SUM(a.hours), 0) AS total_hours "
        "FROM users u "
        "LEFT JOIN absences a "
        " ON a.studentid = u.id "
        " AND a.subjectid = ? "
        " AND a.semesterid = ? "
        "WHERE u.role = 'student' AND u.groupid = ? "
        "GROUP BY u.id, u.name "
        "ORDER BY u.name;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, subjectId);
    sqlite3_bind_int(stmt, 2, semesterId);
    sqlite3_bind_int(stmt, 3, groupId);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int studentId = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        int totalHours = sqlite3_column_int(stmt, 2);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outRows.emplace_back(studentId, name, totalHours);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// ===== Schedule =====

bool Database::addScheduleEntry(int groupId, int subgroup, int weekday,
                                 int lessonNumber, int weekOfCycle,
                                 int subjectId, int teacherId,
                                 const std::string& room, const std::string& lessonType) {
    if (!db) return false;

    int finalGroupId = groupId;
    int finalSubgroup = subgroup;

    // If lecture, make it general (groupid = 0)
    if (lessonType == "ЛК") {
        finalGroupId = 0;
        finalSubgroup = 0;
    }

    const char* sql =
        "INSERT INTO schedule "
        "(groupid, subgroup, weekday, lessonnumber, weekofcycle, "
        "subjectid, teacherid, room, lessontype) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, finalGroupId);
    sqlite3_bind_int(stmt, 2, finalSubgroup);
    sqlite3_bind_int(stmt, 3, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 4, lessonNumber);
    sqlite3_bind_int(stmt, 5, weekOfCycle);
    sqlite3_bind_int(stmt, 6, subjectId);
    sqlite3_bind_int(stmt, 7, teacherId);
    sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, lessonType.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteScheduleEntry(int scheduleId) {
    if (!db) {
        std::cerr << "[✗] deleteScheduleEntry: DB not connected\n";
        return false;
    }

    const char* sql = "DELETE FROM schedule WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] deleteScheduleEntry: prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, scheduleId);
    rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (!ok) {
        std::cerr << "[✗] deleteScheduleEntry: step error: " << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool Database::updateScheduleEntry(int scheduleId, int groupId, int subgroup, int weekday,
                                    int lessonNumber, int weekOfCycle, int subjectId,
                                    int teacherId, const std::string& room, const std::string& lessonType) {
    if (!db) {
        std::cerr << "[✗] updateScheduleEntry: DB not connected\n";
        return false;
    }

    const char* sql = R"(
        UPDATE schedule
        SET groupid = ?,
            subgroup = ?,
            weekday = ?,
            lessonnumber = ?,
            weekofcycle = ?,
            subjectid = ?,
            teacherid = ?,
            room = ?,
            lessontype = ?
        WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] updateScheduleEntry prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, subgroup);
    sqlite3_bind_int(stmt, 3, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 4, lessonNumber);
    sqlite3_bind_int(stmt, 5, weekOfCycle);
    sqlite3_bind_int(stmt, 6, subjectId);
    sqlite3_bind_int(stmt, 7, teacherId);
    sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, lessonType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, scheduleId);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        return true;
    }

    std::cerr << "[✗] updateScheduleEntry: failed to update entry with ID " << scheduleId << "\n";
    return false;
}

bool Database::getScheduleForGroup(
    int groupId,
    int weekday,
    int weekOfCycle,
    std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>& rows)
{
    rows.clear();
    if (!db) {
        std::cerr << "[✗] getScheduleForGroup: DB not connected\n";
        return false;
    }

    const char* sql = R"SQL(
        SELECT
            sch.id,
            sch.lessonnumber,
            sch.subgroup,
            subj.name,
            sch.room,
            COALESCE(sch.lessontype, ''),
            COALESCE(u.name, '') AS teachername
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        LEFT JOIN users u ON sch.teacherid = u.id
        WHERE (sch.groupid = ? OR sch.groupid = 0)
          AND sch.weekday = ?
          AND sch.weekofcycle = ?
        ORDER BY sch.lessonnumber, sch.subgroup
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    const int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getScheduleForGroup: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 3, weekOfCycle);

    int stepRc = 0;
    while ((stepRc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const int lesson = sqlite3_column_int(stmt, 1);
        const int subgroup = sqlite3_column_int(stmt, 2);

        const char* subj = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* room = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* ltype = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* tname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        rows.emplace_back(
            id,
            lesson,
            subgroup,
            subj ? subj : "",
            room ? room : "",
            ltype ? ltype : "",
            tname ? tname : ""
        );
    }

    sqlite3_finalize(stmt);
    return stepRc == SQLITE_DONE;
}

bool Database::getLessonOccurrencesForStudent(
    int studentId,
    int subjectId,
    const std::string& lessonType,
    int semesterId,
    std::vector<LessonOccurrence>& out)
{
    out.clear();
    if (!db) return false;
    if (studentId <= 0 || subjectId <= 0 || semesterId <= 0) return false;

    int groupId = 0;
    int studentSubgroup = 0;
    {
        const char* sql = "SELECT COALESCE(groupid, 0), COALESCE(subgroup, 0) FROM users WHERE id = ? LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, studentId);
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            groupId = sqlite3_column_int(stmt, 0);
            studentSubgroup = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt);
        if (rc != SQLITE_ROW) return false;
    }
    if (groupId <= 0) return false;

    std::string semStart;
    std::string semEnd;
    {
        const char* sql = "SELECT COALESCE(startdate, ''), COALESCE(enddate, '') FROM semesters WHERE id = ? LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, semesterId);
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const unsigned char* s = sqlite3_column_text(stmt, 0);
            const unsigned char* e = sqlite3_column_text(stmt, 1);
            semStart = s ? reinterpret_cast<const char*>(s) : "";
            semEnd = e ? reinterpret_cast<const char*>(e) : "";
        }
        sqlite3_finalize(stmt);
        if (rc != SQLITE_ROW) return false;
    }

    const bool filterBySemesterDates = (!semStart.empty() && !semEnd.empty());

    auto trimCopyLocal = [](std::string s) {
        auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
        while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
        return s;
    };

    const std::string normalizedLessonType = trimCopyLocal(lessonType);
    if (normalizedLessonType.empty()) return false;

    std::vector<std::tuple<int, int, std::string, std::string>> weeks;
    if (!getCycleWeeks(weeks)) return false;

    std::vector<LessonOccurrence> occurrences;
    std::unordered_map<int, std::pair<int, int>> scheduleMetaCache;
    for (const auto& w : weeks) {
        const int weekId = std::get<0>(w);
        const int weekOfCycle = std::get<1>(w);
        const std::string& weekStart = std::get<2>(w);
        const std::string& weekEnd = std::get<3>(w);

        if (filterBySemesterDates) {
            if (!weekEnd.empty() && weekEnd < semStart) continue;
            if (!weekStart.empty() && weekStart > semEnd) continue;
        }

        for (int weekday = 1; weekday <= 6; ++weekday) {
            std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> sched;
            if (!getScheduleForGroup(groupId, weekday, weekOfCycle, sched)) return false;

            for (const auto& row : sched) {
                const int scheduleId = std::get<0>(row);
                const int pairNumber = std::get<1>(row);
                const int subgroup = std::get<2>(row);
                const std::string type = trimCopyLocal(std::get<5>(row));

                if (type != normalizedLessonType) continue;

                int rowSubjectId = 0;
                int teacherId = 0;
                const auto it = scheduleMetaCache.find(scheduleId);
                if (it != scheduleMetaCache.end()) {
                    rowSubjectId = it->second.first;
                    teacherId = it->second.second;
                } else {
                    const char* sql = "SELECT subjectid, teacherid FROM schedule WHERE id = ? LIMIT 1;";
                    sqlite3_stmt* stmt = nullptr;
                    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
                    sqlite3_bind_int(stmt, 1, scheduleId);
                    const int rc = sqlite3_step(stmt);
                    if (rc == SQLITE_ROW) {
                        rowSubjectId = sqlite3_column_int(stmt, 0);
                        teacherId = sqlite3_column_int(stmt, 1);
                    }
                    sqlite3_finalize(stmt);
                    scheduleMetaCache.emplace(scheduleId, std::make_pair(rowSubjectId, teacherId));
                }

                if (rowSubjectId != subjectId) continue;
                if (!((subgroup == 0) || (studentSubgroup == 0) || (subgroup == studentSubgroup))) continue;

                std::string dateISO;
                if (!getDateForWeekdayByWeekId(weekId, weekday, dateISO)) return false;
                if (dateISO.empty()) continue;
                if (filterBySemesterDates) {
                    if (dateISO < semStart || dateISO > semEnd) continue;
                }

                LessonOccurrence occ;
                occ.lessonDateISO = std::move(dateISO);
                occ.scheduleId = scheduleId;
                occ.teacherId = teacherId;
                occ.pairNumber = pairNumber;
                occurrences.push_back(std::move(occ));
            }
        }
    }

    std::sort(occurrences.begin(), occurrences.end(), [](const LessonOccurrence& a, const LessonOccurrence& b) {
        if (a.lessonDateISO != b.lessonDateISO) return a.lessonDateISO < b.lessonDateISO;
        if (a.pairNumber != b.pairNumber) return a.pairNumber < b.pairNumber;
        return a.scheduleId < b.scheduleId;
    });

    occurrences.erase(
        std::unique(occurrences.begin(), occurrences.end(), [](const LessonOccurrence& a, const LessonOccurrence& b) {
            return a.lessonDateISO == b.lessonDateISO;
        }),
        occurrences.end());

    out = std::move(occurrences);
    return true;
}

bool Database::getScheduleForTeacherGroup(
    int teacherId,
    int groupId,
    std::vector<std::tuple<int, int, int, int, int, std::string>>& rows) {
    rows.clear();

    const char* sql = R"(
        SELECT
            s.id,
            s.subjectid,
            s.weekday,
            s.lessonnumber,
            s.subgroup,
            subj.name
        FROM schedule s
        JOIN subjects subj ON s.subjectid = subj.id
        WHERE s.teacherid = ?
          AND s.groupid = ?
        ORDER BY s.weekday, s.lessonnumber, s.subgroup
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, groupId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId = sqlite3_column_int(stmt, 0);
        int subjectId = sqlite3_column_int(stmt, 1);
        int dayOfWeek = sqlite3_column_int(stmt, 2);
        int pairNumber = sqlite3_column_int(stmt, 3);
        int subgroup = sqlite3_column_int(stmt, 4);
        const unsigned char* subjText = sqlite3_column_text(stmt, 5);
        std::string subjectName = subjText ? reinterpret_cast<const char*>(subjText) : "";

        rows.emplace_back(scheduleId, subjectId, dayOfWeek, pairNumber, subgroup, subjectName);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getScheduleForTeacherGroupWeek(
    int teacherId,
    int groupId,
    int weekOfCycle,
    int studentSubgroup,
    std::vector<std::tuple<int, int, int, int, int, std::string, std::string>>& outRows) {
    outRows.clear();
    if (!isConnected()) return false;

    const char* sql = R"SQL(
        SELECT sch.id,
               sch.subjectid,
               sch.weekday,
               sch.lessonnumber,
               sch.subgroup,
               subj.name,
               COALESCE(sch.lessontype, '')
        FROM schedule sch
        JOIN subjects subj ON sch.subjectid = subj.id
        WHERE sch.teacherid = ?
          AND sch.groupid = ?
          AND sch.weekofcycle = ?
          AND (? = 0 OR sch.subgroup = 0 OR sch.subgroup = ?)
        ORDER BY sch.weekday, sch.lessonnumber, sch.subgroup
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, groupId);
    sqlite3_bind_int(stmt, 3, weekOfCycle);
    sqlite3_bind_int(stmt, 4, studentSubgroup);
    sqlite3_bind_int(stmt, 5, studentSubgroup);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId = sqlite3_column_int(stmt, 0);
        int subjectId = sqlite3_column_int(stmt, 1);
        int weekday = sqlite3_column_int(stmt, 2);
        int lessonNumber = sqlite3_column_int(stmt, 3);
        int subgroup = sqlite3_column_int(stmt, 4);
        const char* subjName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ltype = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        outRows.emplace_back(
            scheduleId,
            subjectId,
            weekday,
            lessonNumber,
            subgroup,
            subjName ? subjName : "",
            ltype ? ltype : ""
        );
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::isScheduleSlotBusy(int groupId, int subgroup,
                                   int weekday, int lessonNumber, int weekOfCycle) {
    if (!db) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE groupid = ?
          AND weekday = ?
          AND lessonnumber = ?
          AND weekofcycle = ?
          AND (subgroup = ? OR subgroup = 0)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, subgroup);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

bool Database::isScheduleEmpty(bool& outEmpty) {
    outEmpty = true;
    if (!db) {
        std::cerr << "[✗] isScheduleEmpty: DB not open\n";
        return false;
    }

    const char* sql = "SELECT COUNT(*) FROM schedule;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] isScheduleEmpty: prepare error: " << sqlite3_errmsg(db) << "\n";
        outEmpty = true;
        return true;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int cnt = sqlite3_column_int(stmt, 0);
        outEmpty = (cnt == 0);
        sqlite3_finalize(stmt);
        return true;
    } else {
        std::cerr << "[✗] isScheduleEmpty: step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
}

// ===== Teacher Availability Checks =====

bool Database::isTeacherBusy(int teacherId, int weekday,
                              int lessonNumber, int weekOfCycle) {
    if (!db) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE teacherid = ?
          AND weekday = ?
          AND lessonnumber = ?
          AND weekofcycle = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

bool Database::isRoomBusy(const std::string& room, int weekday,
                          int lessonNumber, int weekOfCycle) {
    if (!db) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE room = ?
          AND weekday = ?
          AND lessonnumber = ?
          AND weekofcycle = ?
          AND room IS NOT NULL
          AND room <> ''
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, normalizeWeekdayForDb(weekday));
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

// ===== Lessons (Simple Schedule Entries) =====

bool Database::addLesson(int groupId,
                         int subjectId,
                         int teacherId,
                         const std::string& date,
                         int timeSlot) {
    if (!db) return false;

    const char* sql =
        "INSERT INTO lessons (groupid, subjectid, teacherid, date, timeslot) "
        "VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, teacherId);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, timeSlot);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::getLessonsForGroupAndDate(
    int groupId,
    const std::string& date,
    std::vector<std::tuple<int, int, std::string>>& outLessons) {
    outLessons.clear();
    if (!db) return false;

    const char* sql =
        "SELECT l.timeslot, s.id, s.name "
        "FROM lessons l "
        "JOIN subjects s ON s.id = l.subjectid "
        "WHERE l.groupid = ? AND l.date = ? "
        "ORDER BY l.timeslot;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int timeSlot = sqlite3_column_int(stmt, 0);
        int subjectId = sqlite3_column_int(stmt, 1);
        const unsigned char* nameText = sqlite3_column_text(stmt, 2);
        std::string subjectName = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outLessons.emplace_back(timeSlot, subjectId, subjectName);
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

// ===== Cycle Weeks Utilities =====

bool Database::getCycleWeeks(
    std::vector<std::tuple<int, int, std::string, std::string>>& out) {
    out.clear();
    if (!db) return false;

    const char* sql =
        "SELECT id, weekofcycle, startdate, enddate "
        "FROM cycleweeks ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int weekNumber = sqlite3_column_int(stmt, 1);
        const unsigned char* s = sqlite3_column_text(stmt, 2);
        const unsigned char* e = sqlite3_column_text(stmt, 3);
        std::string start = s ? reinterpret_cast<const char*>(s) : "";
        std::string end = e ? reinterpret_cast<const char*>(e) : "";
        out.emplace_back(id, weekNumber, start, end);
    }

    sqlite3_finalize(stmt);
    return true;
}

int Database::getWeekOfCycleForDate(const std::string& dateISO) {
    std::tm tmStart{};
    parseDateISO("2025-09-01", tmStart);
    std::time_t tStart = std::mktime(&tmStart);

    std::tm tmDate{};
    if (!parseDateISO(dateISO, tmDate)) {
        return 1;  // Default if date parsing fails
    }

    std::time_t tDate = std::mktime(&tmDate);

    if (tDate < tStart) return 1;

    long diffDays = static_cast<long>((tDate - tStart) / (60 * 60 * 24));
    long weekIndex = diffDays / 7;
    int weekOfCycle = static_cast<int>((weekIndex % 4) + 1);

    return weekOfCycle;
}

int Database::getWeekIdByDate(const std::string& dateISO) {
    if (!db) return 0;

    const char* sql = R"(
        SELECT id
        FROM cycleweeks
        WHERE startdate <= ? AND enddate >= ?
        ORDER BY startdate
        LIMIT 1
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, dateISO.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, dateISO.c_str(), -1, SQLITE_TRANSIENT);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return id;
}

int Database::getWeekOfCycleByWeekId(int weekId) {
    if (!db) return 0;

    const char* sql =
        "SELECT weekofcycle FROM cycleweeks WHERE id = ? LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    sqlite3_bind_int(stmt, 1, weekId);

    int weekNumber = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        weekNumber = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return weekNumber > 0 ? weekNumber : 0;
}

bool Database::getDateForWeekday(int weekOfCycle, int weekday, std::string& outDateISO) {
    if (!db) return false;

    const char* sql = R"(
        SELECT startdate
        FROM cycleweeks
        WHERE weekofcycle = ?
        ORDER BY id
        LIMIT 1
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, weekOfCycle);

    std::string start;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (s) start = s;
    }

    sqlite3_finalize(stmt);
    if (start.empty()) return false;

    int y, m, d;
    if (sscanf(start.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return false;

    // DB weekday convention: 1..6 (Mon..Sat). startdate in cycleweeks is Monday.
    // Offset in days: (weekday - 1)
    if (weekday < 1 || weekday > 6) return false;
    d += (weekday - 1);

    std::ostringstream oss;
    oss << y << '-'
        << std::setw(2) << std::setfill('0') << m << '-'
        << std::setw(2) << std::setfill('0') << d;

    outDateISO = oss.str();
    return true;
}

bool Database::getDateForWeekdayByWeekId(int weekId, int weekday, std::string& outDateISO) {
    outDateISO.clear();
    if (!db) return false;

    // DB weekday convention: 1..6 (Mon..Sat). startdate in cycleweeks is Monday.
    if (weekday < 1 || weekday > 6) return false;

    const char* sql =
        "SELECT startdate FROM cycleweeks WHERE id = ? LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, weekId);

    std::string start;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (s) start = s;
    }

    sqlite3_finalize(stmt);
    if (start.size() != 10) return false;  // YYYY-MM-DD

    std::tm tmStart{};
    tmStart.tm_year = std::stoi(start.substr(0, 4)) - 1900;
    tmStart.tm_mon = std::stoi(start.substr(5, 2)) - 1;
    tmStart.tm_mday = std::stoi(start.substr(8, 2));
    tmStart.tm_hour = 0;
    tmStart.tm_min = 0;
    tmStart.tm_sec = 0;

    std::time_t t = std::mktime(&tmStart);
    if (t == (std::time_t)-1) return false;

    // Offset in days: (weekday - 1)
    t += static_cast<long>(weekday - 1) * 24 * 60 * 60;

    std::tm* tmRes = std::localtime(&t);
    if (!tmRes) return false;

    std::ostringstream oss;
    oss << (tmRes->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tmRes->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tmRes->tm_mday;

    outDateISO = oss.str();
    return true;
}

// ===== File I/O for Schedule =====

bool Database::loadScheduleFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "❌ Error: file not found " << filePath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    if (execute(sql)) {
        std::cout << "✓ Schedule loaded from file: " << filePath << std::endl;
        return true;
    }

    std::cerr << "❌ Error executing SQL" << std::endl;
    return false;
}


bool Database::loadGroupSchedule(int groupId, const std::string& filePath) {
    std::cout << "📚 Loading schedule for group 420" << (600 + groupId) << "..." << std::endl;
    if (loadScheduleFromFile(filePath)) {
        std::string query = "SELECT COUNT(*) FROM schedule WHERE groupid = " + std::to_string(groupId);
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int count = sqlite3_column_int(stmt, 0);
                std::cout << "✓ Total lessons loaded: " << count << std::endl;
            }
            sqlite3_finalize(stmt);
        }
        return true;
    }
    return false;
}

// ===== DB Statistics & Debug =====

void Database::dumpDbStats() {
    std::cerr << "[DB] file: " << fileName << "\n";
    if (!db) {
        std::cerr << "[DB] not connected\n";
        return;
    }

    struct TableInfo {
        const char* name;
    };

    const TableInfo tables[] = {
        {"users"},
        {"groups"},
        {"cycleweeks"},
        {"subjects"},
        {"schedule"}
    };

    for (const auto& t : tables) {
        std::string sql = "SELECT COUNT(*) FROM ";
        sql += t.name;
        sql += ";";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "[DB] " << t.name << ": error\n";
            sqlite3_finalize(stmt);
            continue;
        }
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const int cnt = sqlite3_column_int(stmt, 0);
            std::cerr << "[DB] " << t.name << ": " << cnt << "\n";
        } else {
            std::cerr << "[DB] " << t.name << ": error\n";
        }
        sqlite3_finalize(stmt);
    }
}

void Database::dumpSchemaAndCounts() {
    std::cerr << "[DB schema] file: " << fileName << "\n";
    if (!db) {
        std::cerr << "[DB schema] not connected\n";
        return;
    }

    std::cerr << "[DB schema] tables:";
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name NOT LIKE 'sqlite_%' "
        "ORDER BY name;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        bool any = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (!name) continue;
            std::cerr << (any ? ", " : " ") << name;
            any = true;
        }
        if (!any) std::cerr << " ";
    } else {
        std::cerr << " ";
    }

    std::cerr << "\n";
    sqlite3_finalize(stmt);

    const char* tables[] = {
        "users",
        "groups",
        "semesters",
        "subjects",
        "teachersubjects",
        "teachergroups",
        "cycleweeks",
        "schedule",
        "grades",
        "gradechanges",
        "absences"
    };

    for (const char* t : tables) {
        std::string sql = "SELECT COUNT(*) FROM ";
        sql += t;
        sql += ";";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cerr << "[DB schema] " << t << ": " << sqlite3_column_int(stmt, 0) << " rows\n";
            } else {
                std::cerr << "[DB schema] " << t << ": error\n";
            }
        } else {
            std::cerr << "[DB schema] " << t << ": error\n";
        }
        sqlite3_finalize(stmt);
    }
}

// ===== Teachers with Subjects =====

bool Database::getTeachersWithSubjects(
    std::vector<std::tuple<int, std::string, std::string>>& outTeachers) {
    outTeachers.clear();
    if (!db) return false;

    const char* sql = R"(
        SELECT
            u.id,
            u.name,
            GROUP_CONCAT(s.name, ', ') AS subjects
        FROM users u
        LEFT JOIN teachersubjects ts ON u.id = ts.teacherid
        LEFT JOIN subjects s ON ts.subjectid = s.id
        WHERE u.role = 'teacher'
        GROUP BY u.id, u.name
        ORDER BY u.name
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const unsigned char* subjText = sqlite3_column_text(stmt, 2);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        std::string subjects = subjText ? reinterpret_cast<const char*>(subjText) : "No subjects";
        outTeachers.emplace_back(id, name, subjects);
    }

    sqlite3_finalize(stmt);
    return true;
}
bool Database::insertUser(const std::string& username,
                          const std::string& password,
                          const std::string& role,
                          const std::string& name,
                          int groupId,
                          int subgroup)
{
    if (!isConnected()) {
        std::cerr << "[✗] insertUser: DB not connected\n";
        return false;
    }

    const char* sql =
        "INSERT INTO users (username, password, role, name, groupid, subgroup) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] insertUser prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, name.c_str(), -1, SQLITE_TRANSIENT);

    if (groupId <= 0) sqlite3_bind_null(stmt, 5);
    else sqlite3_bind_int(stmt, 5, groupId);

    sqlite3_bind_int(stmt, 6, subgroup);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] insertUser step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::deleteUserById(int userId)
{
    if (!isConnected()) {
        std::cerr << "[✗] deleteUserById: DB not connected\n";
        return false;
    }

    const char* sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] deleteUserById prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, userId);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] deleteUserById step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    const int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    return changes > 0;
}

bool Database::getGroupsForTeacher(int teacherId,
                                  std::vector<std::pair<int, std::string>>& outGroups)
{
    outGroups.clear();

    if (!isConnected()) {
        std::cerr << "[✗] getGroupsForTeacher: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT g.id, g.name "
        "FROM teachergroups tg "
        "JOIN groups g ON g.id = tg.groupid "
        "WHERE tg.teacherid = ? "
        "ORDER BY g.id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] getGroupsForTeacher prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacherId);

    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outGroups.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getGroupsForTeacher step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addAbsence(int studentId,
                          int subjectId,
                          int semesterId,
                          int hours,
                          const std::string& date,
                          const std::string& type)
{
    if (!isConnected()) {
        std::cerr << "[✗] addAbsence: DB not connected\n";
        return false;
    }

    const char* sql =
        "INSERT INTO absences (studentid, subjectid, semesterid, hours, date, type) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] addAbsence prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);
    sqlite3_bind_int(stmt, 4, hours);
    sqlite3_bind_text(stmt, 5, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, type.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] addAbsence step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getStudentAbsencesForSemester(
    int studentId,
    int semesterId,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outAbsences)
{
    outAbsences.clear();

    if (!isConnected()) {
        std::cerr << "[✗] getStudentAbsencesForSemester: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT s.name, a.hours, COALESCE(a.date, ''), COALESCE(a.type, '') "
        "FROM absences a "
        "JOIN subjects s ON a.subjectid = s.id "
        "WHERE a.studentid = ? AND a.semesterid = ? "
        "ORDER BY a.date;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] getStudentAbsencesForSemester prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, semesterId);

    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* subjText = sqlite3_column_text(stmt, 0);
        const int hours = sqlite3_column_int(stmt, 1);
        const unsigned char* dateText = sqlite3_column_text(stmt, 2);
        const unsigned char* typeText = sqlite3_column_text(stmt, 3);

        const std::string subject = subjText ? reinterpret_cast<const char*>(subjText) : "";
        const std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        const std::string type = typeText ? reinterpret_cast<const char*>(typeText) : "";

        outAbsences.emplace_back(subject, hours, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentAbsencesForSemester step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getStudentTotalAbsences(int studentId, int semesterId, int& outTotalHours)
{
    outTotalHours = 0;

    if (!isConnected()) {
        return false;
    }

    const char* sql =
        "SELECT COALESCE(SUM(hours), 0) "
        "FROM absences "
        "WHERE studentid = ? AND semesterid = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, semesterId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outTotalHours = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::updateGrade(int gradeId,
                           int newValue,
                           const std::string& newDate,
                           const std::string& newType)
{
    if (!isConnected()) {
        std::cerr << "[✗] updateGrade: DB not connected\n";
        return false;
    }

    const char* sql =
        "UPDATE grades "
        "SET value = ?, date = ?, gradetype = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] updateGrade prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, newValue);
    sqlite3_bind_text(stmt, 2, newDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, newType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, gradeId);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        return true;
    }

    return false;
}

bool Database::getGradesForStudentSubject(
    int studentId,
    int subjectId,
    int semesterId,
    std::vector<std::tuple<int, int, std::string, std::string>>& outGrades)
{
    outGrades.clear();

    if (!isConnected()) {
        std::cerr << "[✗] getGradesForStudentSubject: DB not connected\n";
        return false;
    }

    const char* sql =
        "SELECT id, value, COALESCE(date, ''), COALESCE(gradetype, '') "
        "FROM grades "
        "WHERE studentid = ? AND subjectid = ? AND semesterid = ? "
        "ORDER BY date;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] getGradesForStudentSubject prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, studentId);
    sqlite3_bind_int(stmt, 2, subjectId);
    sqlite3_bind_int(stmt, 3, semesterId);

    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const int gradeId = sqlite3_column_int(stmt, 0);
        const int value = sqlite3_column_int(stmt, 1);
        const unsigned char* dateText = sqlite3_column_text(stmt, 2);
        const unsigned char* typeText = sqlite3_column_text(stmt, 3);

        const std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        const std::string type = typeText ? reinterpret_cast<const char*>(typeText) : "";

        outGrades.emplace_back(gradeId, value, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getGradesForStudentSubject step error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

