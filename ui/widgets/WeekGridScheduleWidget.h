#pragma once

#include <QWidget>
#include <QStringList>

class QGridLayout;
class QScrollArea;
class Database;

class WeekGridScheduleWidget : public QWidget {
    Q_OBJECT
public:
    explicit WeekGridScheduleWidget(QWidget* parent = nullptr);

    void setSchedule(Database* db,
                     int groupId,
                     int weekOfCycle,
                     int resolvedWeekId,
                     int currentSubgroup);

private:
    QScrollArea* scrollArea = nullptr;
    QWidget* contentWidget = nullptr;
    QGridLayout* grid = nullptr;

    static QString dayHeaderText(const QString& dayName, const QString& dateISO);
    static QString dateISOForDay(Database* db, int weekOfCycle, int resolvedWeekId, int weekday);
    static bool isRowVisibleForSubgroup(int rowSubgroup, int selectedSubgroup);

    void clearGrid();
};
