#include "../database.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>
 #include <memory>

class ScheduleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Create a test database in memory
        db = std::make_unique<Database>(":memory:");
        ASSERT_TRUE(db->connect());
        ASSERT_TRUE(db->execute("PRAGMA foreign_keys = ON;"));
        ASSERT_TRUE(db->initialize());

        // Seed minimal data
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO groups (id, name) VALUES "
            "(1, '420601'), (2, '420602'), (3, '420603'), (4, '420604');"
        ));
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO users (id, username, password, role, name, groupid, subgroup) VALUES "
            "(101, 'teacher1', '123', 'teacher', 'Преподаватель 1', NULL, 0), "
            "(102, 'teacher2', '123', 'teacher', 'Преподаватель 2', NULL, 0);"
        ));
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO subjects (id, name) VALUES "
            "(1, 'Математика'), (2, 'Физика'), (3, 'Программирование');"
        ));
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
    sqlite3_stmt* stmt = nullptr;
    ASSERT_EQ(
        sqlite3_prepare_v2(db->getHandle(),
                           "SELECT COUNT(*) FROM schedule WHERE lessontype = 'ЛК' AND groupid != 0;",
                           -1, &stmt, nullptr),
        SQLITE_OK
    );

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
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
    sqlite3_stmt* stmt = nullptr;
    ASSERT_EQ(
        sqlite3_prepare_v2(
            db->getHandle(),
            "SELECT weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lessontype, COUNT(*) as cnt "
            "FROM schedule "
            "WHERE lessontype = 'ЛК' AND groupid = 0 "
            "GROUP BY weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lessontype "
            "HAVING cnt > 1;",
            -1, &stmt, nullptr
        ),
        SQLITE_OK
    );
    
    std::vector<std::string> duplicates;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::stringstream ss;
        ss << "Duplicate lecture found: "
           << "weekday=" << sqlite3_column_int(stmt, 0) 
           << ", lessonnumber=" << sqlite3_column_int(stmt, 1)
           << ", weekofcycle=" << sqlite3_column_int(stmt, 2)
           << ", subjectid=" << sqlite3_column_int(stmt, 3)
           << ", teacherid=" << sqlite3_column_int(stmt, 4)
           << ", room=" << (const char*)sqlite3_column_text(stmt, 5)
           << ", lessontype=" << (const char*)sqlite3_column_text(stmt, 6)
           << " (count=" << sqlite3_column_int(stmt, 7) << ")";
        
        duplicates.push_back(ss.str());
    }
    sqlite3_finalize(stmt);
    
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
    sqlite3_stmt* stmt1 = nullptr;
    ASSERT_EQ(
        sqlite3_prepare_v2(db->getHandle(),
                           "SELECT COUNT(*) FROM schedule WHERE groupid = 1 AND (lessontype = 'ПЗ' OR lessontype = 'ЛР');",
                           -1, &stmt1, nullptr),
        SQLITE_OK
    );
    
    int count1 = 0;
    if (sqlite3_step(stmt1) == SQLITE_ROW) {
        count1 = sqlite3_column_int(stmt1, 0);
    }
    sqlite3_finalize(stmt1);
    
    EXPECT_GT(count1, 0) << "Group 1 should have practicals/labs";
    
    // Check that group 2 has its specific classes
    sqlite3_stmt* stmt2 = nullptr;
    ASSERT_EQ(
        sqlite3_prepare_v2(db->getHandle(),
                           "SELECT COUNT(*) FROM schedule WHERE groupid = 2 AND (lessontype = 'ПЗ' OR lessontype = 'ЛР');",
                           -1, &stmt2, nullptr),
        SQLITE_OK
    );
    
    int count2 = 0;
    if (sqlite3_step(stmt2) == SQLITE_ROW) {
        count2 = sqlite3_column_int(stmt2, 0);
    }
    sqlite3_finalize(stmt2);
    
    EXPECT_GT(count2, 0) << "Group 2 should have practicals/labs";
}

// Test that lectures are not present in groups 420602-420604
TEST_F(ScheduleTest, NoLecturesInOtherGroups) {
    // Load group schedules
    db->loadGroupSchedule(2, "schedule_420602_newest.sql");
    db->loadGroupSchedule(3, "schedule_420603_newest.sql");
    db->loadGroupSchedule(4, "schedule_420604_newest.sql");
    
    // Check for any lectures in these groups
    sqlite3_stmt* stmt = nullptr;
    ASSERT_EQ(
        sqlite3_prepare_v2(db->getHandle(),
                           "SELECT COUNT(*) FROM schedule WHERE groupid IN (2, 3, 4) AND lessontype = 'ЛК';",
                           -1, &stmt, nullptr),
        SQLITE_OK
    );
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    
    EXPECT_EQ(count, 0) << "Found lectures in groups 420602-420604";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
