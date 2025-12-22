#include "database.h"
#include <iostream>
#include <string>
#include <sqlite3.h>
#include <vector>
#include <utility>
#include <tuple>
#include <fstream>
#include <sstream>
#include <iomanip>


namespace {

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

}


Database::Database(const std::string& file)
    : db_(nullptr), fileName(file)
{
}

Database::~Database()
{
    disconnect();
}

bool Database::connect()
{
    if (db_ != nullptr) {
        return true; // уже открыта
    }

    int rc = sqlite3_open(fileName.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка открытия SQLite БД: "
                  << (sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "unknown")
                  << std::endl;
        db_ = nullptr;
        return false;
    }

    // Включаем внешние ключи (если используешь связи между таблицами)
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    std::cout << "[✓] SQLite БД успешно открыта: " << fileName << std::endl;
    return true;
}

bool Database::isConnected() const
{
    return db_ != nullptr;
}

void Database::disconnect()
{
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
        std::cout << "[✓] SQLite соединение закрыто." << std::endl;
    }
}

bool Database::execute(const std::string& sql)
{
    if (!isConnected()) {
        std::cerr << "[✗] Нельзя выполнить запрос: БД не открыта." << std::endl;
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка SQLite: "
                  << (errMsg ? errMsg : "unknown")
                  << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
} // ← здесь закрываем execute И БОЛЬШЕ НИЧЕГО

bool Database::initialize()
{
    if (!db_) {
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
        "  start_date TEXT,"
        "  end_date   TEXT"
        ");"

        // users
        "CREATE TABLE IF NOT EXISTS users ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  password TEXT NOT NULL,"
        "  role     TEXT NOT NULL CHECK (role IN ('admin','teacher','student')),"
        "  name     TEXT NOT NULL,"
        "  group_id  INTEGER,"
        "  sub_group INTEGER DEFAULT 0,"
        "  FOREIGN KEY (group_id) REFERENCES groups(id)"
        ");"

        // subjects
        "CREATE TABLE IF NOT EXISTS subjects ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE"
        ");"

        // teacher_subjects
        "CREATE TABLE IF NOT EXISTS teacher_subjects ("
        "  teacher_id INTEGER NOT NULL,"
        "  subject_id INTEGER NOT NULL,"
        "  PRIMARY KEY (teacher_id, subject_id),"
        "  FOREIGN KEY (teacher_id) REFERENCES users(id),"
        "  FOREIGN KEY (subject_id) REFERENCES subjects(id)"
        ");"

        // teacher_groups
        "CREATE TABLE IF NOT EXISTS teacher_groups ("
        "  teacher_id INTEGER NOT NULL,"
        "  group_id    INTEGER NOT NULL,"
        "  PRIMARY KEY (teacher_id, group_id),"
        "  FOREIGN KEY (teacher_id) REFERENCES users(id),"
        "  FOREIGN KEY (group_id)   REFERENCES groups(id)"
        ");"


    // schedule
    "CREATE TABLE IF NOT EXISTS schedule ("
    "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  group_id      INTEGER NOT NULL,"
    "  sub_group     INTEGER NOT NULL,"
    "  weekday      INTEGER NOT NULL,"
    "  lesson_number INTEGER NOT NULL,"
    "  week_of_cycle  INTEGER NOT NULL CHECK(week_of_cycle BETWEEN 1 AND 4),"  // ← ТАК
    "  subject_id    INTEGER NOT NULL,"
    "  teacher_id    INTEGER NOT NULL,"
    "  room         TEXT,"
    "  lesson_type   TEXT,"
    "  FOREIGN KEY (group_id)   REFERENCES groups(id),"
    "  FOREIGN KEY (subject_id) REFERENCES subjects(id),"
    "  FOREIGN KEY (teacher_id) REFERENCES users(id)"
    ");"



        "CREATE INDEX IF NOT EXISTS idx_schedule_group_week "
        "ON schedule(group_id, weekday, week_of_cycle);"
        "CREATE INDEX IF NOT EXISTS idx_schedule_teacher "
        "ON schedule(teacher_id);"
        "CREATE INDEX IF NOT EXISTS idx_schedule_room "
        "ON schedule(room);"

        // grades
        "CREATE TABLE IF NOT EXISTS grades ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  student_id  INTEGER NOT NULL,"
        "  subject_id  INTEGER NOT NULL,"
        "  semester_id INTEGER NOT NULL,"
        "  value      INTEGER NOT NULL CHECK(value BETWEEN 0 AND 10),"
        "  date       TEXT,"
    "  grade_type TEXT,"
        "  FOREIGN KEY (student_id)  REFERENCES users(id),"
        "  FOREIGN KEY (subject_id)  REFERENCES subjects(id),"
        "  FOREIGN KEY (semester_id) REFERENCES semesters(id)"
        ");"

        // gradechanges
        "CREATE TABLE IF NOT EXISTS gradechanges ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  grade_id    INTEGER NOT NULL,"
        "  oldvalue   INTEGER,"
        "  newvalue   INTEGER,"
        "  changedby  INTEGER NOT NULL,"
        "  change_date TEXT NOT NULL,"
        "  comment    TEXT,"
        "  FOREIGN KEY (grade_id)   REFERENCES grades(id),"
        "  FOREIGN KEY (changedby) REFERENCES users(id)"
        ");"

        // cycle_weeks
        "CREATE TABLE IF NOT EXISTS cycle_weeks ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  week_of_cycle INTEGER NOT NULL CHECK(week_of_cycle BETWEEN 1 AND 4),"
        "  start_date    TEXT NOT NULL,"
        "  end_date      TEXT NOT NULL"
        ");"

        // absences
        "CREATE TABLE IF NOT EXISTS absences ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  student_id   INTEGER NOT NULL,"
        "  subject_id   INTEGER NOT NULL,"
        "  semester_id  INTEGER NOT NULL,"
        "  hours       INTEGER NOT NULL,"
        "  date        TEXT,"
        "  type        TEXT,"
        "  FOREIGN KEY (student_id)  REFERENCES users(id),"
        "  FOREIGN KEY (subject_id) REFERENCES subjects(id),"
        "  FOREIGN KEY (semester_id) REFERENCES semesters(id)"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SQLite] " << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    std::cout << "[✓] Структура БД инициализирована.\n";
    return true;
}


bool Database::initializeDemoData() {
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
        "DELETE FROM teacher_subjects;"
        "DELETE FROM teacher_groups;"
        // потом основных пользователей, предметы, группы, недели
        "DELETE FROM users;"
        "DELETE FROM subjects;"
        "DELETE FROM groups;"
        "DELETE FROM cycle_weeks;"
        // сбрасываем AUTOINCREMENT
        "DELETE FROM sqlite_sequence WHERE name IN ("
        " 'users','subjects','groups','schedule',"
        " 'grades','gradechanges','absences','cycle_weeks'"
        ");"

        // ===== НЕДЕЛИ ЦИКЛА =====
        "INSERT INTO cycle_weeks (id, week_of_cycle, start_date, end_date) VALUES"
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
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
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
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
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
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
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
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
        " ('s420604_01', '123', 'student', 'Александренков Денис Алексеевич', 4, 1),"
        " ('s420604_02', '123', 'student', 'Амельченко Роман Игоревич', 4, 1),"
        " ('s420604_03', '123', 'student', 'Борисюк Илья Александрович', 4, 1),"
        " ('s420604_04', '123', 'student', 'Винников Иван Александрович', 4, 1),"
        " ('s420604_05', '123', 'student', 'Гайдукевич Ксения Андреевна', 4, 1),"
        " ('s420604_06', '123', 'student', 'Гузова Александра Евгеньевна', 4, 1),"
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
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
        " ('admin', '123', 'admin', 'Administrator', NULL, 0);"

        // ===== ПРЕПОДАВАТЕЛИ =====
        "INSERT INTO users (username, password, role, name, group_id, sub_group) VALUES "
        " ('t_batin',        '123', 'teacher', 'Батин Н. В.',        NULL, 0),"
        " ('t_bobrova',      '123', 'teacher', 'Боброва Т. С.',      NULL, 0),"
        " ('t_shilin',       '123', 'teacher', 'Шилин Л. Ю.',        NULL, 0),"
        " ('t_nehaychik',    '123', 'teacher', 'Нехайчик Е. В.',     NULL, 0),"
        " ('t_sevurnev',     '123', 'teacher', 'Севернёв А. М.',     NULL, 0),"
        " ('t_sluzhalik',    '123', 'teacher', 'Служалик В. Ю.',     NULL, 0),"
        " ('t_deriabina',    '123', 'teacher', 'Дерябина М. Ю.',     NULL, 0),"
        " ('t_gurevich',     '123', 'teacher', 'Гуревич О. В.',      NULL, 0),"
        " ('t_sharonova',    '123', 'teacher', 'Шаронова Е. И.',     NULL, 0),"
        " ('t_ryshkel',      '123', 'teacher', 'Рышкель О. С.',      NULL, 0),"
        " ('t_tsavlovskaya', '123', 'teacher', 'Цявловская Н. В.',   NULL, 0),"
        " ('t_prigara',      '123', 'teacher', 'Пригара В. Н.',      NULL, 0),"
        " ('t_yuchkov',      '123', 'teacher', 'Ючков А. К.',        NULL, 0),"
        " ('t_german',       '123', 'teacher', 'Герман О. В.',       NULL, 0),"
        " ('t_puhir',        '123', 'teacher', 'Пухир Г. А.',        NULL, 0),"
        " ('t_krishchenovich','123','teacher','Крищенович В. А.',   NULL, 0),"
        " ('t_lappo',        '123', 'teacher', 'Лаппо А. И.',        NULL, 0),"
        " ('t_nehlebova',    '123', 'teacher', 'Нехлебова О. Ю.',    NULL, 0),"
        " ('t_ezovit',       '123', 'teacher', 'Езовит А. В.',       NULL, 0),"
        " ('t_trofimovich',  '123', 'teacher', 'Трофимович А. Ф.',   NULL, 0),"
        " ('t_melnik',       '123', 'teacher', 'Мельник А. А.',      NULL, 0),"
        " ('t_phys',         '123', 'teacher', 'Физкультуров Ф. Ф.', NULL, 0);"

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

    // teacher_subjects (кто какой предмет ведёт)
"INSERT INTO teacher_subjects (teacher_id, subject_id) VALUES "
// АПЭЦ (id=1): Шилин (лк+лр1), Пригара (лр2), Нехайчик (лр3-4)
"(120, 1), "  // tshilin → АПЭЦ
"(129, 1), "  // tprigara → АПЭЦ
"(121, 1), "  // tnehaychik → АПЭЦ

// Физкультура (id=3): физкультуров
"(139, 3), "  // tphys → Физкультура

// ТГ (id=4): Севернев (лк), Служалик (пз)
"(122, 4), "  // tsevurnev → ТГ
"(123, 4), "  // tsluzhalik → ТГ

// ВМиКА (id=5): Батин (лк), Боброва (лр)
"(118, 5), "  // tbatin → ВМиКА
"(119, 5), "  // tbobrova → ВМиКА

// БЖЧ (id=6): Рышкель (лк+пз1-2+лр1-2), Цявловская (пз3-4+лр3-4)
"(127, 6), "  // tryshkel → БЖЧ
"(128, 6), "  // ttsavlovskaya → БЖЧ

// ООП (id=7): Герман (лк), Ючков (лр)
"(131, 7), "  // tgerman → ООП
"(130, 7), "  // tyuchkov → ООП

// ОИнфБ (id=8): Пухир (лк), Крищенович (пз)
"(132, 8), "  // tpuhir → ОИнфБ
"(133, 8), "  // tkrishchenovich → ОИнфБ

// БД (id=9): Лаппо (лк+лр), Нехлебова (пз)
"(134, 9), "  // tlappo → БД
"(135, 9), "  // tnehlebova → БД

// МСиСвИТ (id=10): Дерябина (лк+пз)
"(124, 10), " // tderiabina → МСиСвИТ

// Инф.час + К.Ч (id=11,12): Езовит(гр1), Ючков(гр2), Трофимович(гр3), Мельник(гр4)
"(136, 11), " // tezovit → Инф.час
"(136, 12), " // tezovit → К.Ч
"(130, 11), " // tyuchkov → Инф.час
"(130, 12), " // tyuchkov → К.Ч
"(137, 11), " // ttrofimovich → Инф.час
"(137, 12), " // ttrofimovich → К.Ч
"(138, 11), " // tmelnik → Инф.час
"(138, 12); " // tmelnik → К.Ч

    // ===== СЕМЕСТРЫ =====
    "INSERT INTO semesters (id, name, start_date, end_date) VALUES "
    " (1, '1 семестр 2025/2026', '2025-09-01', '2025-12-31');"
        ;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка заполнения демо-данными: "
                  << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

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
            "INSERT OR IGNORE INTO teacher_groups(teacher_id, group_id) VALUES (?, ?);";

        sqlite3_stmt* insertStmt = nullptr;
        if (sqlite3_prepare_v2(db_, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка подготовки INSERT teacher_groups: "
                      << (sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "unknown") << "\n";
            return false;
        }

        for (const auto& tg : teacherGroups) {
            const std::string& username = tg.first;
            const int teacherId = getUserIdByUsername(db_, username);
            if (teacherId <= 0) {
                sqlite3_finalize(insertStmt);
                sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
                std::cerr << "[✗] teacher_groups: не найден пользователь teacher username='"
                          << username << "'\n";
                return false;
            }

            for (const auto& groupName : tg.second) {
                const int groupId = getGroupIdByName(db_, groupName);
                if (groupId <= 0) {
                    sqlite3_finalize(insertStmt);
                    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
                    std::cerr << "[✗] teacher_groups: не найдена группа name='"
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
                    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
                    std::cerr << "[✗] teacher_groups: ошибка INSERT для teacher='"
                              << username << "', group='" << groupName << "': "
                              << (sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "unknown") << "\n";
                    return false;
                }
            }
        }

        sqlite3_finalize(insertStmt);
    }

    {
        const char* verifySql =
            "SELECT u.username, u.name, GROUP_CONCAT(g.name, ', ') "
            "FROM teacher_groups tg "
            "JOIN users u ON u.id = tg.teacher_id "
            "JOIN groups g ON g.id = tg.group_id "
            "GROUP BY u.id "
            "ORDER BY u.username;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, verifySql, -1, &stmt, nullptr) != SQLITE_OK) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка подготовки SELECT teacher_groups verify: "
                      << (sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "unknown") << "\n";
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
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::cerr << "[✗] Ошибка выполнения SELECT teacher_groups verify: "
                      << (sqlite3_errmsg(db_) ? sqlite3_errmsg(db_) : "unknown") << "\n";
            return false;
        }

        sqlite3_finalize(stmt);
    }

    rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] Ошибка COMMIT демо-данных: "
                  << (errMsg ? errMsg : "unknown") << "\n";
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    std::cout << "[✓] БД заполнена демо-данными для групп 420601–420604.\n";
    std::cout << "[✓] Студентов: 116, Преподавателей: 22, Админ: 1\n";
    return true;
}




bool Database::findUser(const std::string& username,
                        const std::string& password,
                        int& outId,
                        std::string& outName,
                        std::string& outRole)
{
    if (!db_) {
        std::cerr << "[!] findUser: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "SELECT id, name, role "
        "FROM users "
        "WHERE username = ? AND password = ? "
        "LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[!] findUser: sqlite3_prepare_v2 error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    // Привязываем параметры
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    // Выполняем запрос
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outId   = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const unsigned char* roleText = sqlite3_column_text(stmt, 2);

        outName = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outRole = roleText ? reinterpret_cast<const char*>(roleText) : "";

        sqlite3_finalize(stmt);
        return true;
    } else if (rc == SQLITE_DONE) {
        // Пользователь не найден
        sqlite3_finalize(stmt);
        return false;
    } else {
        std::cerr << "[!] findUser: sqlite3_step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
}

bool Database::getAllSemesters(std::vector<std::pair<int, std::string>>& outSemesters)
{
    outSemesters.clear();

    if (!db_) {
        std::cerr << "[✗] getAllSemesters: соединение с БД не открыто\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM semesters ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllSemesters: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSemesters.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllSemesters: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getSubjectsForTeacher(int teacher_id,
                                     std::vector<std::pair<int, std::string>>& outSubjects)
{
    outSubjects.clear();
    if (!db_) {
        std::cerr << "[✗] getSubjectsForTeacher: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "SELECT s.id, s.name "
        "FROM teacher_subjects ts "
        "JOIN subjects s ON ts.subject_id = s.id "
        "WHERE ts.teacher_id = ? "
        "ORDER BY s.id;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getSubjectsForTeacher: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacher_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSubjects.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getSubjectsForTeacher: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllGroups(std::vector<std::pair<int, std::string>>& outGroups)
{
    outGroups.clear();

    if (!db_) {
        std::cerr << "[✗] getAllGroups: соединение с БД не открыто\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM groups ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllGroups: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outGroups.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllGroups: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}
bool Database::getAllSubjects(std::vector<std::pair<int, std::string>>& outSubjects)
{
    outSubjects.clear();
    if (!db_) {
        std::cerr << "[✗] getAllSubjects: соединение с БД не открыто\n";
        return false;
    }

    const char* sql = "SELECT id, name FROM subjects ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllSubjects: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outSubjects.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllSubjects: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllTeachers(std::vector<std::pair<int, std::string>>& outTeachers)
{
    outTeachers.clear();
    if (!db_) {
        std::cerr << "[✗] getAllTeachers: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "SELECT id, name FROM users "
        "WHERE role = 'teacher' "
        "ORDER BY name;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllTeachers: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outTeachers.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllTeachers: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getAllUsers(
    std::vector<std::tuple<int, std::string, std::string, std::string, int, int>>& outUsers)
{
    outUsers.clear();
    if (!db_) {
        std::cerr << "[✗] getAllUsers: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
     "SELECT id, username, name, role, "
     "COALESCE(group_id, 0), "
     "COALESCE(sub_group, 0) "
     "FROM users "
     "ORDER BY id;";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getAllUsers: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* uText = sqlite3_column_text(stmt, 1);
        const unsigned char* nText = sqlite3_column_text(stmt, 2);
        const unsigned char* rText = sqlite3_column_text(stmt, 3);
        int group_id  = sqlite3_column_int(stmt, 4);
        int sub_group = sqlite3_column_int(stmt, 5);

        std::string username = uText ? reinterpret_cast<const char*>(uText) : "";
        std::string name     = nText ? reinterpret_cast<const char*>(nText) : "";
        std::string role     = rText ? reinterpret_cast<const char*>(rText) : "";

        outUsers.emplace_back(id, username, name, role, group_id, sub_group);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getAllUsers: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}


bool Database::getStudentGradesForSemester(
    int studentId,
    int semesterId,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outGrades
) {
    outGrades.clear();
    if (!db_) {
        std::cerr << "[✗] getStudentGradesForSemester: БД не подключена\n";
        return false;
    }

    const char* sql =
        "SELECT s.name, g.value, COALESCE(g.date, ''), COALESCE(g.grade_type, '') "
        "FROM grades g "
        "JOIN subjects s ON g.subject_id = s.id "
        "WHERE g.student_id = ? AND g.semester_id = ? "
        "ORDER BY s.name, g.date";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentGradesForSemester prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
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
        std::string date    = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string gtype   = typeText ? reinterpret_cast<const char*>(typeText) : "";

        outGrades.emplace_back(subject, value, date, gtype);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentGradesForSemester step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getStudentsOfGroup(int group_id,
                                  std::vector<std::pair<int, std::string>>& outStudents)
{
    outStudents.clear();

    if (!db_) {
        std::cerr << "[✗] getStudentsOfGroup: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "SELECT id, name "
        "FROM users "
        "WHERE role = 'student' AND group_id = ? "
        "ORDER BY name;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentsOfGroup: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outStudents.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentsOfGroup: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}
bool Database::getStudentSubjectGrades(
    int student_id,
    int subject_id,
    int semester_id,
    std::vector<std::tuple<int, std::string, std::string>>& outGrades)
{
    outGrades.clear();

    if (!db_) {
        std::cerr << "[✗] getStudentSubjectGrades: соединение с БД не открыто\n";
        return false;
    }

    // В grades нет поля type, берём только value и date,
    // а тип в третьем элементе кортежа будем возвращать пустой строкой.
    const char* sql =
     "SELECT value, COALESCE(date, ''), COALESCE(grade_type, '') "  // ← было type
     "FROM grades "
     "WHERE student_id = ? AND subject_id = ? AND semester_id = ? "
     "ORDER BY date";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentSubjectGrades: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, semester_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int value = sqlite3_column_int(stmt, 0);
        const unsigned char* dateText = sqlite3_column_text(stmt, 1);

        std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string type; // пустая строка, так как в grades нет type

        outGrades.emplace_back(value, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentSubjectGrades: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addGrade(int student_id, int subject_id, int semester_id,
                        int value, const std::string& date, const std::string& grade_type) {
    if (!db_) {
        std::cerr << "[✗] addGrade: БД не подключена\n";
        return false;
    }
    const char* sql =
        "INSERT INTO grades (student_id, subject_id, semester_id, value, date, grade_type) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] addGrade prepare error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, semester_id);
    sqlite3_bind_int(stmt, 4, value);
    sqlite3_bind_text(stmt, 5, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, grade_type.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] addGrade step error: " << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool Database::insertUser(const std::string& username,
                          const std::string& password,
                          const std::string& role,
                          const std::string& name,
                          int group_id,
                          int sub_group)
{
    if (!db_) {
        std::cerr << "[✗] insertUser: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "INSERT INTO users (username, password, role, name, group_id, sub_group) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] insertUser: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, name.c_str(), -1, SQLITE_TRANSIENT);

    if (group_id == 0)
        sqlite3_bind_null(stmt, 5);
    else
        sqlite3_bind_int(stmt, 5, group_id);

    sqlite3_bind_int(stmt, 6, sub_group); // 0 для старших/админов, 1/2 для студентов

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] insertUser: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}



bool Database::deleteUserById(int userId)
{
    if (!db_) {
        std::cerr << "[✗] deleteUserById: соединение с БД не открыто\n";
        return false;
    }

    const char* sql = "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] deleteUserById: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, userId);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] deleteUserById: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    if (changes == 0) {
        std::cout << "Пользователь с таким ID не найден.\n";
        return false;
    }

    return true;
}

bool Database::updateGrade(int grade_id, int newValue, const std::string& newDate,
                           const std::string& newType) {
    if (!db_) {
        std::cerr << "[✗] updateGrade: БД не подключена\n";
        return false;
    }
    const char* sql =
        "UPDATE grades SET value = ?, date = ?, grade_type = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] updateGrade prepare error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1, newValue);
    sqlite3_bind_text(stmt, 2, newDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, newType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, grade_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db_) > 0) {
        return true;
    }
    std::cout << "[!] updateGrade: Оценка с ID " << grade_id << " не найдена.\n";
    return false;
}

bool Database::getGradesForStudentSubject(
    int student_id,
    int subject_id,
    int semester_id,
    std::vector<std::tuple<int, int, std::string, std::string>>& outGrades)
{
    outGrades.clear();

    if (!db_) {
        std::cerr << "[✗] getGradesForStudentSubject: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
    "SELECT id, value, COALESCE(date, ''), COALESCE(grade_type, '') "  // ← grade_type
    "FROM grades "
    "WHERE student_id = ? AND subject_id = ? AND semester_id = ? "
    "ORDER BY date";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getGradesForStudentSubject: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, semester_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int grade_id = sqlite3_column_int(stmt, 0);
        int value   = sqlite3_column_int(stmt, 1);
        const unsigned char* dateText = sqlite3_column_text(stmt, 2);
        const unsigned char* typeText = sqlite3_column_text(stmt, 3);

        std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string type = typeText ? reinterpret_cast<const char*>(typeText) : "";

        outGrades.emplace_back(grade_id, value, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getGradesForStudentSubject: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addAbsence(int student_id,
                          int subject_id,
                          int semester_id,
                          int hours,
                          const std::string& date,
                          const std::string& type)
{
    if (!isConnected()) {
        std::cerr << "[✗] addAbsence: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
    "INSERT INTO absences (student_id, subject_id, semester_id, hours, date, type) "
    "VALUES (?, ?, ?, ?, ?, ?);";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] addAbsence: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, semester_id);
    sqlite3_bind_int(stmt, 4, hours);
    sqlite3_bind_text(stmt, 5, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, type.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] addAbsence: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}



bool Database::getStudentAbsencesForSemester(
    int student_id,
    int semester_id,
    std::vector<std::tuple<std::string, int, std::string, std::string>>& outAbsences)
{
    outAbsences.clear();
    if (!isConnected()) {
        std::cerr << "[✗] getStudentAbsencesForSemester: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
     "SELECT s.name, a.hours, COALESCE(a.date, ''), COALESCE(a.type, '') "
     "FROM absences a "
     "JOIN subjects s ON a.subject_id = s.id "
     "WHERE a.student_id = ? AND a.semester_id = ? "
     "ORDER BY a.date;";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentAbsencesForSemester: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, semester_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* subjText = sqlite3_column_text(stmt, 0);
        int hours                     = sqlite3_column_int(stmt, 1);
        const unsigned char* dateText = sqlite3_column_text(stmt, 2);
        const unsigned char* typeText = sqlite3_column_text(stmt, 3);

        std::string subj = subjText ? reinterpret_cast<const char*>(subjText) : "";
        std::string date = dateText ? reinterpret_cast<const char*>(dateText) : "";
        std::string type = typeText ? reinterpret_cast<const char*>(typeText) : "";

        outAbsences.emplace_back(subj, hours, date, type);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getStudentAbsencesForSemester: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}




bool Database::getStudentTotalAbsences(int student_id,
                                       int semester_id,
                                       int& outTotalHours)
{
    outTotalHours = 0;
    if (!isConnected()) return false;

    const char* sql =
        "SELECT SUM(hours) FROM absences "
        "WHERE student_id = ? AND semester_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, semester_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outTotalHours = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return true;
}
bool Database::getStudentUnexcusedAbsences(int student_id,
                                           int semester_id,
                                           int& outUnexcusedHours)
{
    outUnexcusedHours = 0;
    if (!isConnected()) return false;

    const char* sql =
        "SELECT SUM(hours) FROM absences "
        "WHERE student_id = ? AND semester_id = ? AND type = 'unexcused';";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, semester_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outUnexcusedHours = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::deleteTodayAbsence(int student_id,
                                  int subject_id,
                                  int semester_id,
                                  const std::string& date)
{
    if (!isConnected()) return false;

    const char* sql =
        "DELETE FROM absences "
        "WHERE student_id = ? AND subject_id = ? AND semester_id = ? AND date = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, student_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, semester_id);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    return changes > 0;
}

bool Database::addTeacherGroup(int teacher_id, int group_id) {
    if (!isConnected()) return false;

    const char* sql =
        "INSERT OR IGNORE INTO teacher_groups (teacher_id, group_id) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, teacher_id);
    sqlite3_bind_int(stmt, 2, group_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::getGroupsForTeacher(int teacher_id,
                                   std::vector<std::pair<int, std::string>>& outGroups) {
    outGroups.clear();
    if (!isConnected()) return false;

    const char* sql =
        "SELECT g.id, g.name "
        "FROM teacher_groups tg "
        "JOIN groups g ON g.id = tg.group_id "
        "WHERE tg.teacher_id = ? "
        "ORDER BY g.id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, teacher_id);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outGroups.emplace_back(id, name);
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}
bool Database::addLesson(int group_id,
                         int subject_id,
                         int teacher_id,
                         const std::string& date,
                         int time_slot )
{
    if (!isConnected()) return false;

    const char* sql =
        "INSERT INTO lessons (group_id, subject_id, teacher_id, date, time_slot) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, subject_id);
    sqlite3_bind_int(stmt, 3, teacher_id);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, time_slot );

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::getLessonsForGroupAndDate(
        int group_id,
        const std::string& date,
        std::vector<std::tuple<int,int,std::string>>& outLessons)
{
    outLessons.clear();
    if (!isConnected()) return false;

    const char* sql =
        "SELECT l.time_slot, s.id, s.name "
        "FROM lessons l "
        "JOIN subjects s ON s.id = l.subject_id "
        "WHERE l.group_id = ? AND l.date = ? "
        "ORDER BY l.time_slot;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int time_slot  = sqlite3_column_int(stmt, 0);
        int subject_id = sqlite3_column_int(stmt, 1);
        const unsigned char* nameText = sqlite3_column_text(stmt, 2);
        std::string subjectName = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outLessons.emplace_back(time_slot , subject_id, subjectName);
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::getGroupSubjectAbsencesSummary(
    int group_id,
    int subject_id,
    int semester_id,
    std::vector<std::tuple<int, std::string, int>>& outRows)
{
    outRows.clear();
    if (!db_) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
     "SELECT u.id, u.name, COALESCE(SUM(a.hours), 0) AS total_hours "
     "FROM users u "
     "LEFT JOIN absences a "
     "  ON a.student_id  = u.id "
     " AND a.subject_id  = ? "
     " AND a.semester_id = ? "
     "WHERE u.role = 'student' AND u.group_id = ? "
     "GROUP BY u.id, u.name "
     "ORDER BY u.name;";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, subject_id);
    sqlite3_bind_int(stmt, 2, semester_id);
    sqlite3_bind_int(stmt, 3, group_id);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int student_id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        int totalHours = sqlite3_column_int(stmt, 2);

        std::string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        outRows.emplace_back(student_id, name, totalHours);
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getGroupSubjectAbsencesSummary: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addScheduleEntry(int group_id, int sub_group, int weekday,
                                int lesson_number, int week_of_cycle,
                                int subject_id, int teacher_id,
                                const std::string& room) {
    const char* sql =
        "INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, sub_group);
    sqlite3_bind_int(stmt, 3, weekday);
    sqlite3_bind_int(stmt, 4, lesson_number);
    sqlite3_bind_int(stmt, 5, week_of_cycle);
    sqlite3_bind_int(stmt, 6, subject_id);
    sqlite3_bind_int(stmt, 7, teacher_id);
    sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}


bool Database::getScheduleForGroup(int group_id, int weekday, int week_of_cycle,
    std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>>& rows)
{
    rows.clear();

    const char* sql = R"(
    SELECT
    sch.id,
    sch.lesson_number,
    sch.sub_group,
    subj.name,
    sch.room,
    COALESCE(sch.lesson_type, ''),
    u.name AS teacher_name
FROM schedule sch
JOIN subjects subj ON sch.subject_id = subj.id
JOIN users    u    ON sch.teacher_id = u.id
WHERE (sch.group_id = ? OR sch.group_id = 0)
  AND sch.weekday = ?
  AND sch.week_of_cycle = ?
ORDER BY sch.lesson_number, sch.sub_group

)";



    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getScheduleForGroup: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, week_of_cycle);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id        = sqlite3_column_int(stmt, 0);
        int lesson    = sqlite3_column_int(stmt, 1);
        int sub_group  = sqlite3_column_int(stmt, 2);
        const char* subj = (const char*)sqlite3_column_text(stmt, 3);
        const char* room = (const char*)sqlite3_column_text(stmt, 4);
        const char* ltype= (const char*)sqlite3_column_text(stmt, 5);
        const char* tname= (const char*)sqlite3_column_text(stmt, 6);

        rows.emplace_back(
            id,
            lesson,
            sub_group,
            subj ? subj : "",
            room ? room : "",
            ltype ? ltype : "",
            tname ? tname : ""
        );
    }

    sqlite3_finalize(stmt);
    return true;
}



bool Database::getStudentGroupAndSubgroup(int student_id,
                                          int& out_group_id,
                                          int& out_sub_group)
{
    out_group_id = 0;
    out_sub_group = 0;

    if (!db_) {
        std::cerr << "[✗] getStudentGroupAndSubgroup: соединение с БД не открыто\n";
        return false;
    }

    const char* sql =
        "SELECT COALESCE(group_id, 0), COALESCE(sub_group, 0) "
        "FROM users "
        "WHERE id = ? AND role = 'student' "
        "LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getStudentGroupAndsub_group: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, student_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out_group_id  = sqlite3_column_int(stmt, 0);
        out_sub_group = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return true;
    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false; // не студент или нет такой записи
    } else {
        std::cerr << "[✗] getStudentGroupAndsub_group: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
}
bool Database::getScheduleForTeacherGroup(
    int teacher_id,
    int group_id,
    std::vector<std::tuple<int,int,int,int,int,std::string>>& rows
) {
    rows.clear();
    const char* sql = R"(
        SELECT
    s.id,
    s.subject_id,
    s.weekday,
    s.lesson_number,
    s.sub_group,
    subj.name
FROM schedule s
JOIN subjects subj ON s.subject_id = subj.id
WHERE s.teacher_id = ?
  AND s.group_id = ?
ORDER BY s.weekday, s.lesson_number, s.sub_group

    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, teacher_id);
    sqlite3_bind_int(stmt, 2, group_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int scheduleId = sqlite3_column_int(stmt, 0);
        int subject_id  = sqlite3_column_int(stmt, 1);
        int dayOfWeek  = sqlite3_column_int(stmt, 2);
        int pairNumber = sqlite3_column_int(stmt, 3);
        int sub_group   = sqlite3_column_int(stmt, 4);
        const unsigned char* subjText = sqlite3_column_text(stmt, 5);
        std::string subjectName = subjText ? reinterpret_cast<const char*>(subjText) : "";
        rows.emplace_back(scheduleId, subject_id, dayOfWeek, pairNumber, sub_group, subjectName);
    }

    sqlite3_finalize(stmt);
    return true;
}


bool Database::isScheduleEmpty(bool& outEmpty) {
    outEmpty = true;
    if (!db_) {
        std::cerr << "[✗] isScheduleEmpty: БД не открыта\n";
        return false;
    }

    const char* sql = "SELECT COUNT(*) FROM schedule;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        // Если таблицы schedule ещё нет, считаем, что расписание пустое
        std::cerr << "[✗] isScheduleEmpty: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
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
        std::cerr << "[✗] isScheduleEmpty: step error: "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_finalize(stmt);
        return false;
    }
}

bool Database::loadScheduleFromFile(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "❌ Ошибка: не найден файл " << filePath << std::endl;
        return false;
    }

    // Читаем весь файл в строку
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();
    file.close();

    // Выполняем SQL
    if (execute(sql)) {
        std::cout << "✓ Расписание загружено из файла: " << filePath << std::endl;
        return true;
    } else {
        std::cerr << "❌ Ошибка при выполнении SQL" << std::endl;
        return false;
    }
}

bool Database::loadGroupSchedule(int group_id, const std::string& filePath) {
    std::cout << "📚 Загрузка расписания для группы 420" << (600 + group_id) << "..." << std::endl;

    if (loadScheduleFromFile(filePath)) {
        // Проверяем количество пар в расписании
        std::string query = "SELECT COUNT(*) FROM schedule WHERE group_id = " + std::to_string(group_id);
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int count = sqlite3_column_int(stmt, 0);
                std::cout << "✓ Всего пар загружено: " << count << std::endl;
            }
            sqlite3_finalize(stmt);
        }

        return true;
    }
    return false;
}

// Вспомогательная функция: переводит строку "YYYY-MM-DD" в std::tm
static bool parseDateISO(const std::string& s, std::tm& out) {
    if (s.size() != 10) return false;
    std::memset(&out, 0, sizeof(out));
    out.tm_year = std::stoi(s.substr(0, 4)) - 1900;
    out.tm_mon  = std::stoi(s.substr(5, 2)) - 1;   // 0–11
    out.tm_mday = std::stoi(s.substr(8, 2));       // 1–31
    out.tm_hour = 0;
    out.tm_min  = 0;
    out.tm_sec  = 0;
    std::mktime(&out); // нормализуем
    return true;
}

// int Database::getweekofcycleForDate(const std::string& dateISO) {
//     // старт семестра: 2025-09-01
//     std::tm tmStart{};
//     parseDateISO("2025-09-01", tmStart);
//     std::time_t tStart = std::mktime(&tmStart);
//
//     std::tm tmDate{};
//     if (!parseDateISO(dateISO, tmDate)) {
//         return 1; // дефолт, если дата кривая
//     }
//     std::time_t tDate = std::mktime(&tmDate);
//
//     if (tDate < tStart) return 1;
//
//     // разница в днях
//     long diffDays = static_cast<long>((tDate - tStart) / (60 * 60 * 24));
//
//     // номер недели от начала семестра (0=первая неделя)
//     long weekIndex = diffDays / 7;
//
//     // цикл 1–4
//     int week_of_cycle = static_cast<int>((weekIndex % 4) + 1);
//     return week_of_cycle;
// }

bool Database::getCycleWeeks(
    std::vector<std::tuple<int,int,std::string,std::string>>& out) {
    out.clear();
    if (!db_) return false;

    const char* sql =
     "SELECT id, week_of_cycle, start_date, end_date "
     "FROM cycle_weeks ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id          = sqlite3_column_int(stmt, 0);
        int wcycle      = sqlite3_column_int(stmt, 1);
        const unsigned char* s = sqlite3_column_text(stmt, 2);
        const unsigned char* e = sqlite3_column_text(stmt, 3);
        std::string start = s ? reinterpret_cast<const char*>(s) : "";
        std::string end   = e ? reinterpret_cast<const char*>(e) : "";
        out.emplace_back(id, wcycle, start, end);
    }
    sqlite3_finalize(stmt);
    return true;
}
int Database::getWeekOfCycleByDate(const std::string& dateISO){
    if (!db_) {
        std::cerr << "[✗] getWeekOfCycleByDate: соединение с БД не открыто\n";
        return 0;
    }

    const char* sql =
        "SELECT week_of_cycle FROM cycle_weeks "
        "WHERE start_date <= ? AND end_date >= ? "
        "LIMIT 1;";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] getweek_of_cycleByDate: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return 0;
    }

    sqlite3_bind_text(stmt, 1, dateISO.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, dateISO.c_str(), -1, SQLITE_TRANSIENT);

    int week_of_cycle = 0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        week_of_cycle = sqlite3_column_int(stmt, 0);
    } else if (rc != SQLITE_DONE) {
        std::cerr << "[✗] getweek_of_cycleByDate: step error: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return week_of_cycle;
}

bool Database::getDateForWeekday(int week_of_cycle, int weekday, std::string& outDateISO) {
    if (!db_) return false;
    const char* sql = R"(
SELECT start_date
FROM cycle_weeks
WHERE week_of_cycle = ?
ORDER BY start_date
LIMIT 1
)";


    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, week_of_cycle);

    std::string start;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char* s = (const char*)sqlite3_column_text(stmt, 0);
        if (s) start = s;
    }
    sqlite3_finalize(stmt);

    if (start.empty()) return false;

    int y, m, d;
    if (sscanf(start.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return false;

    d += weekday; // 0..5, предположим внутри одной недели

    std::ostringstream oss;
    oss << y << '-'
        << std::setw(2) << std::setfill('0') << m << '-'
        << std::setw(2) << std::setfill('0') << d;

    outDateISO = oss.str();
    return true;
}

bool Database::isScheduleSlotBusy(int group_id, int sub_group,
                                  int weekday, int lesson_number, int week_of_cycle) {
    if (!db_) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE group_id = ?
  AND weekday = ?
  AND lesson_number = ?
  AND week_of_cycle = ?
  AND (sub_group = ? OR sub_group = 0)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lesson_number);
    sqlite3_bind_int(stmt, 4, week_of_cycle);
    sqlite3_bind_int(stmt, 5, sub_group);
    sqlite3_bind_int(stmt, 6, sub_group);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count > 0;
}

// bool Database::addScheduleEntry(int group_id, int sub_group, int weekday, int lesson_number,
//                                 int week_of_cycle, int subject_id, int teacher_id,
//                                 const std::string& room, const std::string& lesson_type ) {
//     if (!db_) return false;
//
//     int realgroup_id = group_id;
//     int realsub_group = sub_group;
//     if (lesson_type == "ЛК") {
//         realgroup_id = 0;
//         realsub_group = 0;
//     }
//
//
//     const char* sql = R"(
//         INSERT INTO schedule
//             (group_id, sub_group, weekday, lesson_number,
//              week_of_cycle, subject_id, teacher_id, room, lesson_type)
//         VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
//     )";
//
//     sqlite3_stmt* stmt = nullptr;
//     int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
//     if (rc != SQLITE_OK) {
//         std::cerr << "[✗] addScheduleEntry: " << sqlite3_errmsg(db_) << "\n";
//         return false;
//     }
//
//     sqlite3_bind_int(stmt, 1, realgroup_id);
//     sqlite3_bind_int(stmt, 2, realsub_group);
//     sqlite3_bind_int(stmt, 3, weekday);
//     sqlite3_bind_int(stmt, 4, lesson_number);
//     sqlite3_bind_int(stmt, 5, week_of_cycle);
//     sqlite3_bind_int(stmt, 6, subject_id);
//     sqlite3_bind_int(stmt, 7, teacher_id);
//     sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);
//     sqlite3_bind_text(stmt, 9, lesson_type .c_str(), -1, SQLITE_TRANSIENT);
//
//     rc = sqlite3_step(stmt);
//     bool ok = (rc == SQLITE_DONE);
//     sqlite3_finalize(stmt);
//     return ok;
// }

bool Database::getScheduleForGroupWeek(
    int group_id, int week_of_cycle,
    std::vector<std::tuple<int,int,int,int,
                           std::string,std::string,
                           std::string,std::string>>& rows) {
    rows.clear();
    if (!db_) return false;

    const char* sql = R"(
        SELECT sch.id, sch.weekday, sch.lesson_number, sch.sub_group,
       s.name as subject_name,
       u.name as teacher_name,
       sch.room,
       sch.lesson_type
FROM schedule sch
JOIN subjects s ON sch.subject_id = s.id
JOIN users u ON sch.teacher_id = u.id
WHERE sch.group_id = ?
  AND sch.week_of_cycle = ?
  AND (sch.sub_group = 0 OR sch.sub_group = ?)  // Добавляем фильтр по подгруппе
ORDER BY sch.weekday, sch.lesson_number, sch.sub_group
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, week_of_cycle);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id        = sqlite3_column_int(stmt, 0);
        int weekday   = sqlite3_column_int(stmt, 1);
        int lesson    = sqlite3_column_int(stmt, 2);
        int sub_group  = sqlite3_column_int(stmt, 3);
        const char* subj = (const char*)sqlite3_column_text(stmt, 4);
        const char* teach= (const char*)sqlite3_column_text(stmt, 5);
        const char* room = (const char*)sqlite3_column_text(stmt, 6);
        const char* ltype= (const char*)sqlite3_column_text(stmt, 7);

        rows.emplace_back(
            id,
            weekday,
            lesson,
            sub_group,
            subj   ? subj   : "",
            teach  ? teach  : "",
            room   ? room   : "",
            ltype  ? ltype  : ""
        );
    }

    sqlite3_finalize(stmt);
    return true;
}


bool Database::addLectureForAllGroups(int basegroup_id, int sub_group,
                                      int weekday, int lesson_number, int week_of_cycle,
                                      int subject_id, int teacher_id,
                                      const std::string& room, const std::string& lesson_type ) {
    if (!db_) return false;
    if (lesson_type != "ЛК" || sub_group != 0) {
        return true; // не лекция или не общая — ничего не делаем
    }

    // получаем все группы
    std::vector<std::pair<int,std::string>> groups;
    if (!getAllGroups(groups) || groups.empty()) {
        return false;
    }

    bool okAll = true;
    for (const auto& g : groups) {
        int gid = g.first;
        if (gid == basegroup_id) continue;

        // не затираем, если там уже что-то стоит
        if (isScheduleSlotBusy(gid, sub_group, weekday, lesson_number, week_of_cycle)) {
            continue;
        }

        if (!addScheduleEntry(gid, sub_group, weekday, lesson_number,
                              week_of_cycle, subject_id, teacher_id,
                              room, lesson_type )) {
            okAll = false;
                              }
    }
    return okAll;
}

bool Database::deleteScheduleEntry(int scheduleId) {
    if (!db_) {
        std::cerr << "[✗] deleteScheduleEntry: БД не открыта\n";
        return false;
    }

    const char* sql = "DELETE FROM schedule WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] deleteScheduleEntry: prepare error: "
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, scheduleId);

    rc = sqlite3_step(stmt);
    bool ok = (rc == SQLITE_DONE);
    if (!ok) {
        std::cerr << "[✗] deleteScheduleEntry: step error: "
                  << sqlite3_errmsg(db_) << "\n";
    }

    sqlite3_finalize(stmt);
    return ok;
}

// ===== ПРОВЕРКА ЗАНЯТОСТИ ПРЕПОДАВАТЕЛЯ =====
bool Database::isTeacherBusy(int teacher_id, int weekday,
                              int lesson_number, int week_of_cycle) {
    if (!db_) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE teacher_id = ?
<<<<<<< HEAD
  AND weekday = ?
  AND lesson_number = ?
  AND week_of_cycle = ?
  AND (id <> ? OR ? = 0)  // Более понятный порядок условий
=======
          AND weekday = ?
          AND lesson_number = ?
          AND week_of_cycle = ?
>>>>>>> parent of b351274 (закрыл блок А)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, teacher_id);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lesson_number);
    sqlite3_bind_int(stmt, 4, week_of_cycle);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

// ===== ПРОВЕРКА ЗАНЯТОСТИ АУДИТОРИИ =====
bool Database::isRoomBusy(const std::string& room, int weekday,
                          int lesson_number, int week_of_cycle) {
    if (!db_) return false;

    const char* sql = R"(
        SELECT COUNT(*)
        FROM schedule
        WHERE room = ?
<<<<<<< HEAD
  AND weekday = ?
  AND lesson_number = ?
  AND week_of_cycle = ?
  AND room IS NOT NULL
  AND room <> ''
  AND (id <> ? OR ? = 0)  // Более понятный порядок условий
=======
          AND weekday = ?
          AND lesson_number = ?
          AND week_of_cycle = ?
>>>>>>> parent of b351274 (закрыл блок А)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lesson_number);
    sqlite3_bind_int(stmt, 4, week_of_cycle);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

// ===== ПОЛУЧИТЬ ПРЕПОДАВАТЕЛЕЙ С ПРЕДМЕТАМИ =====
bool Database::getTeachersWithSubjects(
    std::vector<std::tuple<int, std::string, std::string>>& outTeachers) {

    outTeachers.clear();
    if (!db_) return false;

    const char* sql = R"(
        SELECT
            u.id,
            u.name,
            GROUP_CONCAT(s.name, ', ') AS subjects
        FROM users u
        LEFT JOIN teacher_subjects ts ON u.id = ts.teacher_id
        LEFT JOIN subjects s ON ts.subject_id = s.id
        WHERE u.role = 'teacher'
        GROUP BY u.id, u.name
        ORDER BY u.name
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const unsigned char* subjText = sqlite3_column_text(stmt, 2);

        std::string name = nameText ?
            reinterpret_cast<const char*>(nameText) : "";
        std::string subjects = subjText ?
            reinterpret_cast<const char*>(subjText) : "Нет предметов";

        outTeachers.emplace_back(id, name, subjects);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool Database::addScheduleEntry(int group_id, int sub_group, int weekday, int lesson_number,
                                int week_of_cycle, int subject_id, int teacher_id,
                                const std::string& room, const std::string& lesson_type ) {
    if (!db_) return false;

    // Если лекция — записываем её как общую (group_id = 0, sub_group = 0)
    int finalgroup_id = group_id;
    int finalsub_group = sub_group;
    if (lesson_type == "ЛК") {
        finalgroup_id = 0;
        finalsub_group = 0;
    }

    const char* sql = R"(
        INSERT INTO schedule
        (group_id, sub_group, weekday, lesson_number, week_of_cycle,
         subject_id, teacher_id, room, lesson_type)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, finalgroup_id);
    sqlite3_bind_int(stmt, 2, finalsub_group);
    sqlite3_bind_int(stmt, 3, weekday);
    sqlite3_bind_int(stmt, 4, lesson_number);
    sqlite3_bind_int(stmt, 5, week_of_cycle);
    sqlite3_bind_int(stmt, 6, subject_id);
    sqlite3_bind_int(stmt, 7, teacher_id);
    sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, lesson_type .c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::deleteGrade(int grade_id) {
    if (!db_) {
        std::cerr << "[✗] deleteGrade: БД не подключена\n";
        return false;
    }
    const char* sql = "DELETE FROM grades WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[✗] deleteGrade prepare error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }
    sqlite3_bind_int(stmt, 1, grade_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db_) > 0) {
        return true;
    }
    return false;
}
bool Database::updateScheduleEntry(int schedule_id, int group_id, int sub_group, int weekday,
                                   int lesson_number, int week_of_cycle, int subject_id,
                                   int teacher_id, const std::string& room, const std::string& lesson_type) {
    if (!db_) {
        std::cerr << "[✗] updateScheduleEntry: БД не подключена\n";
        return false;
    }

    const char* sql = R"(
        UPDATE schedule
SET group_id = ?,
    sub_group = ?,
    weekday = ?,
    lesson_number = ?,
    week_of_cycle = ?,
    subject_id = ?,
    teacher_id = ?,
    room = ?,
    lesson_type = ?
WHERE id = ?
  AND NOT EXISTS (
    SELECT 1 FROM schedule
    WHERE group_id = ?
      AND sub_group = ?
      AND weekday = ?
      AND lesson_number = ?
      AND week_of_cycle = ?
      AND id <> ?  // Исключаем текущую запись
  )
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[✗] updateScheduleEntry prepare error: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, sub_group);
    sqlite3_bind_int(stmt, 3, weekday);
    sqlite3_bind_int(stmt, 4, lesson_number);
    sqlite3_bind_int(stmt, 5, week_of_cycle);
    sqlite3_bind_int(stmt, 6, subject_id);
    sqlite3_bind_int(stmt, 7, teacher_id);
    sqlite3_bind_text(stmt, 8, room.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, lesson_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, schedule_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE && sqlite3_changes(db_) > 0) {
        return true;
    }

    std::cerr << "[✗] updateScheduleEntry: не удалось обновить запись с ID " << schedule_id << "\n";
    return false;
}

