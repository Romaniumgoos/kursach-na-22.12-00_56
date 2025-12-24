#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "database.h"

class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LoginWindow(Database* db, QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void onLoginClicked();

private:
    Database* db_;

    // UI элементы
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* loginButton_;
    QLabel* statusLabel_;

    void setupUI();
    void openStudentWindow(int userId, const QString& userName);
    void openTeacherWindow(int userId, const QString& userName);
    void openAdminWindow(int userId, const QString& userName);
};

#endif // LOGINWINDOW_H
