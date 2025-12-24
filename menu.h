
// menu.h
#pragma once
#include "database.h"
#include <string>
#include <array>

struct WeekSelection {
    int weekId = 0;       // id в cycleweeks, если выбрана календарная неделя
    int weekOfCycle = 0;  // 1..4 – то, что хранится в schedule.weekofcycle
};
WeekSelection decodeWeekSelection(Database& db, int sel)
{
    WeekSelection r;
    if (sel == 0) return r; // отмена

    if (sel < 0) {
        // mode 1: введён номер недели цикла 1..4
        r.weekOfCycle = -sel;
        r.weekId = 0;
        return r;
    }

    // sel > 0 => это weekId (mode 2/3)
    r.weekId = sel;
    r.weekOfCycle = db.getWeekOfCycleByWeekId(r.weekId);
    return r;
}

const std::array<const char*, 6>& getDayNames();    // weekday 0..5

const std::array<const char*, 6>& getPairTimes();   // lesson_number 1..6 -> index 0..5
std::string formatDateLabel(const std::string& iso); // "YYYY-MM-DD" -> "DD-MM"

std::string makeISODateFromDayMonth(const std::string& ddmm, int year);
int chooseWeekOfCycleOrDate(Database& db);

// Новые утилиты безопасного ввода
int readIntInRange(const std::string& prompt,
                   int min, int max,
                   int defaultValue,
                   bool allowCancel,
                   int cancelValue);

int readChoiceFromList(const std::string& prompt,
                       int minChoice, int maxChoice,
                       bool allowCancel,
                       int cancelValue);

std::string readToken(const std::string& prompt);
void showGroupSchedule(Database& db, int groupId);
