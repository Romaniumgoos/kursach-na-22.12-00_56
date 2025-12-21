#pragma once

#include "user.h"
#include "database.h"

class Student : public User {
public:
    using User::User;

    void displayMenu(Database& db) override;

    // Просмотр собственных пропусков
    void viewMyAbsences(Database& db);

    // Моё расписание
    bool viewMySchedule(Database& db);

    // Новые методы для расписания
    int getGroupId() const { return groupId_; }
    int getSubgroup() const { return subgroup_; }

    // Инициализация из БД после логина
    bool loadGroupInfo(Database& db);

private:
    int groupId_ = 0;
    int subgroup_ = 0;
};
