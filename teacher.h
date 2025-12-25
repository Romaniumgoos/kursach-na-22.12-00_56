#pragma once

#include "user.h"
#include "database.h"
#include <iostream>
#include <string>
#include <vector>

class Teacher : public User {
private:
    int teacherId;
    Database* db;

public:
    using User::User; // оставляем, если где‑то используется базовый конструктор

    Teacher(int teacherId,
            const std::string& username,
            const std::string& name,
            Database* db);
    // Просмотр СВОЕГО расписания на определённую неделю
    bool viewMySchedule(int weekOfCycle);

    // Просмотр расписания на ВСЕ недели
    bool viewFullSchedule();

    // Твои уже существующие методы
    void displayMenu(Database& db);
    void addAbsence(Database& db);
    void viewStudentAbsences(Database& db, int semesterId = 1);
};
