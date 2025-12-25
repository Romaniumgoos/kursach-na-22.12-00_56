#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include "ui/models/WeekSelection.h"

class Database;

class StudentSchedulePage : public QWidget {
    Q_OBJECT
public:
    explicit StudentSchedulePage(Database* db, int studentId, QWidget* parent = nullptr);

public slots:
    void onPeriodChanged(const WeekSelection& selection);
    void onSubgroupChanged(int subgroup);

private:
    Database* db = nullptr;
    int studentId = 0;
    int currentSubgroup = 0;
    bool subgroupAutoSelected = false;
    WeekSelection currentSelection;

    QTableWidget* table = nullptr;
    QLabel* emptyStateLabel = nullptr;
    QComboBox* subgroupCombo = nullptr;
    QLabel* periodLabel = nullptr;

    void setupLayout();
    void setupTable();
    void loadSchedule();
};
