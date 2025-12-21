#pragma once

#include "user.h"
#include "database.h"
#include <iostream>
#include <string>
#include <vector>

class Admin : public User {
private:
    int admin_id_;      // дублирующий ID, можно использовать getId() из User
    Database* db_;

public:
    // Базовый конструктор User: User(int id, const std::string& name, const std::string& role);
    using User::User;

    // Явный конструктор администратора

    Admin(int admin_id,
          const std::string& username,
          const std::string& name,
          Database* db);

    // ===== НОВЫЕ ФУНКЦИИ ДЛЯ РАСПИСАНИЯ =====

    // Просмотр расписания ЛЮБОЙ ГРУППЫ на определённую неделю
    bool viewScheduleForGroup(int groupId, int subgroup, int weekOfCycle);
    // Просмотр расписания ЛЮБОГО ПРЕПОДАВАТЕЛЯ на определённую неделю
    bool viewScheduleForTeacher(int teacherId, int weekOfCycle);

    // Список всех групп (кратко)
    bool listAllGroups();

    // ===== СУЩЕСТВУЮЩЕЕ МЕНЮ АДМИНА =====
    void displayMenu(Database& db) override;

};
