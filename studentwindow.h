#ifndef STUDENTWINDOW_H
#define STUDENTWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include "database.h"

class StudentWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit StudentWindow(Database* db, int studentId, const QString& studentName, 
                          QWidget *parent = nullptr);
    ~StudentWindow();

private slots:
    void loadGrades();
    void loadAbsences();
    void loadSchedule();
    void onWeekChanged(int index);

private:
    Database* db_;
    int studentId_;
    QString studentName_;
    int groupId_;
    int subgroup_;

    // UI элементы
    QTabWidget* tabWidget_;

    // Вкладка "Оценки"
    QWidget* gradesTab_;
    QTableWidget* gradesTable_;
    QLabel* averageLabel_;

    // Вкладка "Пропуски"
    QWidget* absencesTab_;
    QTableWidget* absencesTable_;
    QLabel* totalAbsencesLabel_;

    // Вкладка "Расписание"
    QWidget* scheduleTab_;
    QComboBox* weekComboBox_;
    QTableWidget* scheduleTable_;

    void setupUI();
    void setupGradesTab();
    void setupAbsencesTab();
    void setupScheduleTab();
    void loadStudentInfo();
};

#endif // STUDENTWINDOW_H
