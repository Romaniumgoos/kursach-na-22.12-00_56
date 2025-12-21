#include "user.h"
#include "database.h"
#include "admin.h"
#include "teacher.h"
#include "student.h"

#include <iostream>
#include <memory>

void User::printProfile() const {
    std::cout << "ID: " << id_
              << ", username: " << username_
              << ", name: " << name_
              << ", role: " << role_ << std::endl;
}

std::shared_ptr<User> User::authenticate(Database& db,
                                         const std::string& username,
                                         const std::string& password)
{
    int id = 0;
    std::string name;
    std::string role;

    if (!db.findUser(username, password, id, name, role)) {
        return nullptr;
    }

    if (role == "admin") {
        // Admin(int id, const std::string& username, const std::string& name, Database* db)
        return std::make_shared<Admin>(id, username, name, &db);
    } else if (role == "teacher") {
        // Teacher(int id, const std::string& username, const std::string& name, Database* db)
        return std::make_shared<Teacher>(id, username, name, &db);
    } else if (role == "student") {
        // Student(int id, const std::string& username, const std::string& name, const std::string& role)
        return std::make_shared<Student>(id, username, name, role);
    }

    return nullptr;
}
