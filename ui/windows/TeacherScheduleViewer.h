#pragma once

#include <QObject>
#include <QWidget>

#include "ui/models/WeekSelection.h"

class Database;
class PeriodSelectorWidget;
class WeekGridScheduleWidget;
class QComboBox;
class QLabel;

class TeacherScheduleViewer : public QWidget {
    Q_OBJECT
public:
    explicit TeacherScheduleViewer(Database* db, int teacherId, const QString& teacherName, QWidget* parent = nullptr);

private slots:
    void onPeriodChanged(const WeekSelection& selection);

private:
    Database* db = nullptr;
    int teacherId = 0;
    QString teacherName;

    int currentSubgroup = 0;
    WeekSelection currentSelection;

    QLabel* titleLabel = nullptr;
    PeriodSelectorWidget* periodSelector = nullptr;
    QComboBox* subgroupCombo = nullptr;
    WeekGridScheduleWidget* weekGrid = nullptr;

    void setupLayout();
    void reload();
};
