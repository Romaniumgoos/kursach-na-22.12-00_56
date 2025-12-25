#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "database.h"
#include "ui/widgets/ThemeToggleWidget.h"
#include <QToolBar>



class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LoginWindow(Database* db, QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void onLoginClicked();

private:
    Database* db;

    // UI элементы
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QPushButton* loginButton;
    QLabel* statusLabel;

    void setupUI();
    void openStudentWindow(int userId, const QString& userName);
    void openTeacherWindow(int userId, const QString& userName);
    void openAdminWindow(int userId, const QString& userName);
};

#endif // LOGINWINDOW_H
