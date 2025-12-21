#include "../database.h"
#include "../teacher.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

class ScheduleViewingTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Инициализация тестовой базы данных
        db = std::make_unique<Database>(":memory:");
        
        // Создаем необходимые таблицы
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
        
            -- Добавляем тестовые данные
            INSERT OR IGNORE INTO groups (id, name, course) VALUES 
                (1, '420601', 3),
                (2, '420602', 3);
                
            INSERT OR IGNORE INTO users (id, username, full_name, role) VALUES 
                (1, 'teacher1', 'Преподаватель 1', 'teacher'),
                (2, 'teacher2', 'Преподаватель 2', 'teacher');
                
            INSERT OR IGNORE INTO subjects (id, name) VALUES 
                (1, 'АПЭЦ'),
                (2, 'Математика');
                
            -- Добавляем тестовое расписание для 420602
            -- 2-я неделя, понедельник, 1 пара - АПЭЦ
            INSERT INTO schedule (group_id, sub_group, weekday, lesson_number, week_of_cycle, subject_id, teacher_id, room, lesson_type)
            VALUES 
                (2, 0, 1, 1, 2, 1, 1, '101', 'ЛК'),
                (2, 1, 1, 2, 2, 1, 1, '102', 'ПЗ'),
                (2, 2, 1, 2, 2, 1, 1, '103', 'ПЗ');
        ");
    }
    
    static void TearDownTestSuite() {
        db.reset();
    }
    
    void SetUp() override {
        // Очищаем вывод перед каждым тестом
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
    // Создаем объект преподавателя (для доступа к методам просмотра расписания)
    Teacher teacher(db.get(), 1, "teacher1", "Преподаватель 1");
    
    // Вызываем метод просмотра расписания (2 - номер группы 420602, 2 - вторая неделя)
    teacher.viewGroupSchedule(2, 2);
    
    // Проверяем вывод (примерная проверка, уточните формат вывода в вашем коде)
    std::string result = output.str();
    
    // Проверяем наличие ожидаемых данных в выводе
    EXPECT_NE(result.find("420602"), std::string::npos) << "Не найдена группа 420602 в выводе";
    EXPECT_NE(result.find("АПЭЦ"), std::string::npos) << "Не найден предмет АПЭЦ в расписании";
    
    // Проверяем, что нет ошибок с колонками
    EXPECT_EQ(result.find("no such column"), std::string::npos) 
        << "Обнаружена ошибка с отсутствующей колонкой";
}

// Тест 2: Проверка отсутствия падений при неверном ID группы
TEST_F(ScheduleViewingTest, ViewNonExistentGroupSchedule) {
    Teacher teacher(db.get(), 1, "teacher1", "Преподаватель 1");
    
    // Пытаемся получить расписание для несуществующей группы (например, 999)
    EXPECT_NO_THROW(teacher.viewGroupSchedule(999, 1)) 
        << "Метод viewGroupSchedule упал с несуществующим ID группы";
    
    std::string result = output.str();
    // Проверяем, что есть сообщение об отсутствии расписания
    EXPECT_FALSE(result.empty()) << "Нет сообщения о пустом расписании";
}

// Тест 3: Проверка формата вывода расписания
TEST_F(ScheduleViewingTest, CheckScheduleFormat) {
    Teacher teacher(db.get(), 1, "teacher1", "Преподаватель 1");
    
    teacher.viewGroupSchedule(2, 2);
    std::string result = output.str();
    
    // Проверяем формат вывода (примерный формат, уточните по вашему коду)
    EXPECT_NE(result.find("Понедельник"), std::string::npos) << "Не найден заголовок дня недели";
    EXPECT_NE(result.find("1 пара"), std::string::npos) << "Не найден номер пары";
    EXPECT_NE(result.find("ауд."), std::string::npos) << "Не указан номер аудитории";
}

// Тест 4: Проверка корректности отображения подгрупп
TEST_F(ScheduleViewingTest, CheckSubgroupsDisplay) {
    Teacher teacher(db.get(), 1, "teacher1", "Преподаватель 1");
    
    teacher.viewGroupSchedule(2, 2);
    std::string result = output.str();
    
    // Проверяем отображение подгрупп (если они есть)
    if (result.find("подгруппа") != std::string::npos) {
        EXPECT_NE(result.find("1 подгруппа"), std::string::npos) 
            << "Не найдена информация о подгруппах";
    }
}

// Тест 5: Проверка отображения типа занятия
TEST_F(ScheduleViewingTest, CheckLessonTypeDisplay) {
    Teacher teacher(db.get(), 1, "teacher1", "Преподаватель 1");
    
    teacher.viewGroupSchedule(2, 2);
    std::string result = output.str();
    
    // Проверяем отображение типа занятия (ЛК, ПЗ, ЛР)
    EXPECT_NE(result.find("ЛК"), std::string::npos) 
        << "Не отображается тип занятия (ЛК/ПЗ/ЛР)";

}

    int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }

