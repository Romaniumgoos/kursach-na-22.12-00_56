#include "../database.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>

class ScheduleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Create a test database in memory
        db = std::make_unique<Database>(":memory:");
        
        // Initialize with test data
        db->execute("PRAGMA foreign_keys = ON;");
        
        // Create necessary tables (simplified version from your schema)
        db->execute(R"(
            CREATE TABLE IF NOT EXISTS groups (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                course INTEGER NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                full_name TEXT NOT NULL,
                role TEXT NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS subjects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS schedule (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                group_id INTEGER NOT NULL,
                sub_group INTEGER NOT NULL,
                weekday INTEGER NOT NULL,
                lesson_number INTEGER NOT NULL,
                week_of_cycle INTEGER NOT NULL,
                subject_id INTEGER NOT NULL,
                teacher_id INTEGER NOT NULL,
                room TEXT NOT NULL,
                lesson_type TEXT NOT NULL,
                FOREIGN KEY (group_id) REFERENCES groups(id),
                FOREIGN KEY (subject_id) REFERENCES subjects(id),
                FOREIGN KEY (teacher_id) REFERENCES users(id)
            );
        ");
        
        // Add test groups
        db->execute("INSERT OR IGNORE INTO groups (name, course) VALUES ('420601', 3), ('420602', 3), ('420603', 3), ('420604', 3);");
        
        // Add test teachers
        db->execute("INSERT OR IGNORE INTO users (username, full_name, role) VALUES 
                    ('teacher1', 'Преподаватель 1', 'teacher'),
                    ('teacher2', 'Преподаватель 2', 'teacher');");
                    
        // Add test subjects
        db->execute("INSERT OR IGNORE INTO subjects (name) VALUES 
                    ('Математика'), ('Физика'), ('Программирование');");
    }
    
    static void TearDownTestSuite() {
        db.reset();
    }
    
    void SetUp() override {
        // Clear schedule before each test
        db->execute("DELETE FROM schedule;");
    }
    
    static std::unique_ptr<Database> db;
};

std::unique_ptr<Database> ScheduleTest::db = nullptr;

// Test that lectures (ЛК) are only loaded once with group_id=0
TEST_F(ScheduleTest, LecturesAreCommon) {
    // Load group schedules
    db->loadGroupSchedule(1, "schedule_420601_newest.sql");
    
    // Check that lectures have group_id=0
    auto stmt = db->prepare("SELECT COUNT(*) FROM schedule WHERE lesson_type = 'ЛК' AND group_id != 0;");
    ASSERT_NE(stmt, nullptr);
    
    int count = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt.get(), 0);
    }
    
    EXPECT_EQ(count, 0) << "Found lectures with group_id != 0";
}

// Test that lectures are not duplicated when loading multiple groups
TEST_F(ScheduleTest, NoDuplicateLectures) {
    // Load all group schedules
    db->loadGroupSchedule(1, "schedule_420601_newest.sql");
    db->loadGroupSchedule(2, "schedule_420602_newest.sql");
    db->loadGroupSchedule(3, "schedule_420603_newest.sql");
    db->loadGroupSchedule(4, "schedule_420604_newest.sql");
    
    // Get count of each lecture
    auto stmt = db->prepare(
        "SELECT weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type, COUNT(*) as cnt "
        "FROM schedule "
        "WHERE lesson_type = 'ЛК' AND group_id = 0 "
        "GROUP BY weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type "
        "HAVING cnt > 1;"
    );
    
    std::vector<std::string> duplicates;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::stringstream ss;
        ss << "Duplicate lecture found: "
           << "weekday=" << sqlite3_column_int(stmt.get(), 0) 
           << ", lesson_number=" << sqlite3_column_int(stmt.get(), 1)
           << ", week_of_cycle=" << sqlite3_column_int(stmt.get(), 2)
           << ", subject_id=" << sqlite3_column_int(stmt.get(), 3)
           << ", teacher_id=" << sqlite3_column_int(stmt.get(), 4)
           << ", room=" << (const char*)sqlite3_column_text(stmt.get(), 5)
           << ", lesson_type=" << (const char*)sqlite3_column_text(stmt.get(), 6)
           << " (count=" << sqlite3_column_int(stmt.get(), 7) << ")";
        
        duplicates.push_back(ss.str());
    }
    
    EXPECT_TRUE(duplicates.empty()) << [&]() {
        std::string msg = "Found " + std::to_string(duplicates.size()) + " duplicate lectures:\n";
        for (const auto& dup : duplicates) {
            msg += "- " + dup + "\n";
        }
        return msg;
    }();
}

// Test that practicals (ПЗ) and labs (ЛР) are loaded for each group
TEST_F(ScheduleTest, GroupSpecificClassesExist) {
    // Load group schedules
    db->loadGroupSchedule(1, "schedule_420601_newest.sql");
    db->loadGroupSchedule(2, "schedule_420602_newest.sql");
    
    // Check that group 1 has its specific classes
    auto stmt1 = db->prepare(
        "SELECT COUNT(*) FROM schedule "
        "WHERE group_id = 1 AND (lesson_type = 'ПЗ' OR lesson_type = 'ЛР');"
    );
    
    int count1 = 0;
    if (sqlite3_step(stmt1.get()) == SQLITE_ROW) {
        count1 = sqlite3_column_int(stmt1.get(), 0);
    }
    
    EXPECT_GT(count1, 0) << "Group 1 should have practicals/labs";
    
    // Check that group 2 has its specific classes
    auto stmt2 = db->prepare(
        "SELECT COUNT(*) FROM schedule "
        "WHERE group_id = 2 AND (lesson_type = 'ПЗ' OR lesson_type = 'ЛР');"
    );
    
    int count2 = 0;
    if (sqlite3_step(stmt2.get()) == SQLITE_ROW) {
        count2 = sqlite3_column_int(stmt2.get(), 0);
    }
    
    EXPECT_GT(count2, 0) << "Group 2 should have practicals/labs";
}

// Test that lectures are not present in groups 420602-420604
TEST_F(ScheduleTest, NoLecturesInOtherGroups) {
    // Load group schedules
    db->loadGroupSchedule(2, "schedule_420602_newest.sql");
    db->loadGroupSchedule(3, "schedule_420603_newest.sql");
    db->loadGroupSchedule(4, "schedule_420604_newest.sql");
    
    // Check for any lectures in these groups
    auto stmt = db->prepare(
        "SELECT COUNT(*) FROM schedule "
        "WHERE group_id IN (2, 3, 4) AND lesson_type = 'ЛК';"
    );
    
    int count = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt.get(), 0);
    }
    
    EXPECT_EQ(count, 0) << "Found lectures in groups 420602-420604";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
