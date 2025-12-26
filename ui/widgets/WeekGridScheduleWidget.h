#pragma once

#include <QObject>
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

    void setTeacherSchedule(Database* db,
                            int teacherId,
                            int groupId,
                            const QString& groupName,
                            int weekOfCycle,
                            int resolvedWeekId,
                            int currentSubgroup);

    void setTeacherScheduleAllGroups(Database* db,
                                    int teacherId,
                                    int weekOfCycle,
                                    int resolvedWeekId,
                                    int currentSubgroup);

signals:
    void lessonClicked(int scheduleId);
    void teacherClicked(int scheduleId);

private:
    QScrollArea* scrollArea = nullptr;
    QWidget* contentWidget = nullptr;
    QGridLayout* grid = nullptr;

    QWidget* selectedLessonCardBody = nullptr;

    static QString dayHeaderText(const QString& dayName, const QString& dateISO);
    static QString dateISOForDay(Database* db, int weekOfCycle, int resolvedWeekId, int weekday);
    static bool isRowVisibleForSubgroup(int rowSubgroup, int selectedSubgroup);

    static void repolish(QWidget* w);

    void clearGrid();
};
