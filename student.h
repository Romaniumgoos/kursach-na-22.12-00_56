#pragma once
#include "user.h"
#include "database.h"

class Student : public User {
public:
    using User::User;

    void displayMenu(Database& db) override;

    void viewMyAbsences(Database& db);
    bool viewMySchedule(Database& db);

    int getGroupId() const { return groupId; }
    int getSubgroup() const { return subgroup; }

    bool loadGroupInfo(Database& db);

private:
    int groupId = 0;
    int subgroup = 0;
};
