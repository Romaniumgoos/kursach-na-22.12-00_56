#pragma once

#include <QString>

// Структура для унификации выбора периода
struct WeekSelection {
    enum Mode { CycleWeek, CalendarWeek, ByDate };

    Mode mode;
    int weekOfCycle = 1;    // 1-4 для режима CycleWeek
    int weekId = 0;         // id из cycleweeks для режима CalendarWeek
    QString selectedDate;   // YYYY-MM-DD для режима ByDate (от неё вычислим неделю)

    WeekSelection(int weekOfCycle_ = 1)
        : mode(CycleWeek), weekOfCycle(weekOfCycle_) {}

    WeekSelection(Mode m, int id)
        : mode(m), weekId(id) {}

    WeekSelection(Mode m, QString date)
        : mode(m), selectedDate(date) {}
};
