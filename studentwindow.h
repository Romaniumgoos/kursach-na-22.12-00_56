#ifndef STUDENTWINDOW_H
#define STUDENTWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include "database.h"
class ThemeToggleWidget;
class StudentWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit StudentWindow(Database* db, int studentId, const QString& studentName, 
                          QWidget *parent = nullptr);
    ~StudentWindow();

private:
    Database* db;
    int studentId;
    QString studentName;

    // UI элементы
    QTabWidget* tabWidget;

    ThemeToggleWidget* themeToggle = nullptr;


    void setupUI();
    void setupScheduleTab();
};

#endif // STUDENTWINDOW_H
