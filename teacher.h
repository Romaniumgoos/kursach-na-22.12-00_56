#pragma once

#include "user.h"
#include "database.h"
#include <iostream>
#include <string>
#include <vector>

class Teacher : public User {
private:
    int teacher_id_;
    Database* db_;

public:
    using User::User; // оставляем, если где‑то используется базовый конструктор

    Teacher(int teacher_id,
            const std::string& username,
            const std::string& name,
            Database* db);
    // Просмотр СВОЕГО расписания на определённую неделю
    bool viewMySchedule(int week_of_cycle);

    // Просмотр расписания на ВСЕ недели
    bool viewFullSchedule();

    // Твои уже существующие методы
    void displayMenu(Database& db);
    void addAbsence(Database& db);
    void viewStudentAbsences(Database& db, int semesterId = 1);
};
