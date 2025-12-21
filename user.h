#pragma once

#include <string>
#include <memory>

class Database; // объявление, чтобы использовать в методах User

class User {
protected:
    int id_{};
    std::string username_;
    std::string name_;
    std::string role_;

public:
    User(int id,
         std::string username,
         std::string name,
         std::string role)
        : id_(id),
          username_(std::move(username)),
          name_(std::move(name)),
          role_(std::move(role)) {}

    virtual ~User() = default;

    int getId() const { return id_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getName() const { return name_; }
    const std::string& getRole() const { return role_; }

    virtual void printProfile() const;

    // Делает User абстрактным
    virtual void displayMenu(Database& db) = 0;

    // Авторизация, вернёт Admin/Teacher/Student
    static std::shared_ptr<User> authenticate(Database& db,
                                              const std::string& username,
                                              const std::string& password);
};
