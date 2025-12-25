#include "../database.h"
#include "../admin.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>
 #include <memory>
 #include <sstream>
 #include <iostream>

class ScheduleViewingTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Инициализация тестовой базы данных
        db = std::make_unique<Database>(":memory:");
        ASSERT_TRUE(db->connect());
        ASSERT_TRUE(db->execute("PRAGMA foreign_keys = ON;"));
        ASSERT_TRUE(db->initialize());

        // Seed minimal data for viewing schedule
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO groups (id, name) VALUES (1, '420601'), (2, '420602');"
        ));
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO users (id, username, password, role, name, groupid, subgroup) VALUES "
            "(1, 'admin', '123', 'admin', 'Administrator', NULL, 0), "
            "(10, 'teacher1', '123', 'teacher', 'Преподаватель 1', NULL, 0);"
        ));
        ASSERT_TRUE(db->execute(
            "INSERT OR IGNORE INTO subjects (id, name) VALUES (1, 'АПЭЦ'), (2, 'Математика');"
        ));

        // Insert schedule rows using camelCase schema
        ASSERT_TRUE(db->execute(
            "INSERT INTO schedule (groupid, subgroup, weekday, lessonnumber, weekofcycle, subjectid, teacherid, room, lessontype) VALUES "
            "(2, 0, 1, 1, 2, 1, 10, '101', 'ЛК'), "
            "(2, 1, 1, 2, 2, 1, 10, '102', 'ПЗ'), "
            "(2, 2, 1, 2, 2, 1, 10, '103', 'ПЗ');"
        ));
    }
    
    static void TearDownTestSuite() {
        db.reset();
    }
    
    void SetUp() override {
        output.str("");
        output.clear();
    }
    
    // Вспомогательная функция для перехвата вывода
    std::stringstream output;
    
    static std::unique_ptr<Database> db;
};

std::unique_ptr<Database> ScheduleViewingTest::db = nullptr;

// Тест 1: Просмотр расписания группы 420602 на 2-ю неделю
TEST_F(ScheduleViewingTest, ViewGroupScheduleWeek2) {
    Admin admin(1, "admin", "Administrator", db.get());

    auto* oldBuf = std::cout.rdbuf(output.rdbuf());
    const bool ok = admin.viewScheduleForGroup(2, /*subgroup=*/0, /*weekOfCycle=*/2);
    std::cout.rdbuf(oldBuf);
    ASSERT_TRUE(ok);

    const std::string result = output.str();
    
    // Проверяем наличие ожидаемых данных в выводе
    EXPECT_NE(result.find("420602"), std::string::npos) << "Не найдена группа 420602 в выводе";
    EXPECT_NE(result.find("АПЭЦ"), std::string::npos) << "Не найден предмет АПЭЦ в расписании";
    
    // Проверяем, что нет ошибок с колонками
    EXPECT_EQ(result.find("no such column"), std::string::npos) 
        << "Обнаружена ошибка с отсутствующей колонкой";
}

// Тест 2: Проверка отсутствия падений при неверном ID группы
TEST_F(ScheduleViewingTest, ViewNonExistentGroupSchedule) {
    Admin admin(1, "admin", "Administrator", db.get());

    auto* oldBuf = std::cout.rdbuf(output.rdbuf());
    EXPECT_NO_THROW({
        admin.viewScheduleForGroup(999, /*subgroup=*/0, /*weekOfCycle=*/1);
    });
    std::cout.rdbuf(oldBuf);

    const std::string result = output.str();
    EXPECT_FALSE(result.empty()) << "Нет сообщения о пустом расписании";
}

// Тест 3: Проверка формата вывода расписания
TEST_F(ScheduleViewingTest, CheckScheduleFormat) {
    Admin admin(1, "admin", "Administrator", db.get());

    auto* oldBuf = std::cout.rdbuf(output.rdbuf());
    admin.viewScheduleForGroup(2, /*subgroup=*/0, /*weekOfCycle=*/2);
    std::cout.rdbuf(oldBuf);

    const std::string result = output.str();
    
    // Проверяем формат вывода (примерный формат, уточните по вашему коду)
    EXPECT_NE(result.find("Понедельник"), std::string::npos) << "Не найден заголовок дня недели";
    EXPECT_NE(result.find("1 пара"), std::string::npos) << "Не найден номер пары";
    EXPECT_NE(result.find("ауд."), std::string::npos) << "Не указан номер аудитории";
}

// Тест 4: Проверка корректности отображения подгрупп
TEST_F(ScheduleViewingTest, CheckSubgroupsDisplay) {
    Admin admin(1, "admin", "Administrator", db.get());

    auto* oldBuf = std::cout.rdbuf(output.rdbuf());
    admin.viewScheduleForGroup(2, /*subgroup=*/0, /*weekOfCycle=*/2);
    std::cout.rdbuf(oldBuf);

    const std::string result = output.str();
    
    // Проверяем отображение подгрупп (если они есть)
    if (result.find("подгруппа") != std::string::npos) {
        EXPECT_NE(result.find("1 подгруппа"), std::string::npos) 
            << "Не найдена информация о подгруппах";
    }
}

// Тест 5: Проверка отображения типа занятия
TEST_F(ScheduleViewingTest, CheckLessonTypeDisplay) {
    Admin admin(1, "admin", "Administrator", db.get());

    auto* oldBuf = std::cout.rdbuf(output.rdbuf());
    admin.viewScheduleForGroup(2, /*subgroup=*/0, /*weekOfCycle=*/2);
    std::cout.rdbuf(oldBuf);

    const std::string result = output.str();
    
    // Проверяем отображение типа занятия (ЛК, ПЗ, ЛР)
    EXPECT_NE(result.find("ЛК"), std::string::npos) 
        << "Не отображается тип занятия (ЛК/ПЗ/ЛР)";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
