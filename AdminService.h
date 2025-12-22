#pragma once
#include "database.h"
#include <vector>
#include <string>

// Структура для хранения данных о занятии
struct ScheduleEntry {
    int id;
    int groupId;        // ID группы
    int subGroup;       // Подгруппа (0 - вся группа, 1 - первая подгруппа, 2 - вторая)
    int weekday;        // День недели (0-пн, 1-вт, ..., 5-сб)
    int lessonNumber;   // Номер пары (1-6)
    int weekOfCycle;    // Неделя цикла (1-4)
    int subjectId;      // ID предмета
    int teacherId;      // ID преподавателя
    std::string room;   // Аудитория
    std::string lessonType; // Тип занятия (ЛК, ПЗ, ЛР)
};

class AdminService {
    Database& db;  // Ссылка на базу данных
public:
    explicit AdminService(Database& database) : db(database) {}
    
    // Управление расписанием
    bool addScheduleEntry(const ScheduleEntry& entry);
    bool updateScheduleEntry(int entryId, const ScheduleEntry& newData);
    bool deleteScheduleEntry(int entryId);
    std::vector<ScheduleEntry> getScheduleForGroup(int groupId, int weekOfCycle);
    
    // Управление группами
    bool createGroup(const std::string& groupName);
    std::vector<std::pair<int, std::string>> getAllGroups();
    
    // Другие методы администрирования...
};