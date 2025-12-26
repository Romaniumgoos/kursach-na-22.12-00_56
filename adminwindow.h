#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "database.h"

class QTabWidget;
class QTableWidget;
class QPushButton;
class QComboBox;
class QLineEdit;
class QWidget;
class WeekGridScheduleWidget;

class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(Database* db, int adminId, const QString& adminName, 
                        QWidget *parent = nullptr);
    ~AdminWindow();

private:
    Database* db;
    int adminId;
    QString adminName;

    QTabWidget* tabWidget = nullptr;

    // Users tab
    QLineEdit* userSearchEdit = nullptr;
    QComboBox* userRoleFilterCombo = nullptr;
    QTableWidget* usersTable = nullptr;
    QPushButton* addUserButton = nullptr;
    QPushButton* editUserButton = nullptr;
    QPushButton* deleteUserButton = nullptr;
    QPushButton* refreshUsersButton = nullptr;

    // Schedule tab
    QComboBox* schedGroupCombo = nullptr;
    QComboBox* schedSubgroupCombo = nullptr;
    QComboBox* schedWeekCombo = nullptr;
    WeekGridScheduleWidget* scheduleGrid = nullptr;
    int selectedScheduleId = 0;
    QPushButton* addScheduleButton = nullptr;
    QPushButton* editScheduleButton = nullptr;
    QPushButton* deleteScheduleButton = nullptr;
    QPushButton* refreshScheduleButton = nullptr;

    void setupUI();

    QWidget* buildUsersTab();
    QWidget* buildScheduleTab();

    void reloadUsers();
    void reloadGroupsInto(QComboBox* combo, bool withAllOption);
    void reloadSchedule();

private slots:
    void onAddUser();
    void onEditUser();
    void onDeleteUser();

    void onAddSchedule();
    void onEditSchedule();
    void onDeleteSchedule();
};

#endif // ADMINWINDOW_H
