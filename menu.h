// menu.h
#pragma once

#include "database.h"

#include <array>
#include <string>

// Выбор недели: либо weekOfCycle (1..4), либо конкретный weekId из cycleweeks
struct WeekSelection {
    int weekId = 0;      // id в cycleweeks, если выбрана календарная неделя
    int weekOfCycle = 0; // 1..4 – то, что хранится в schedule.weekofcycle
};

// ВАЖНО: только объявление (без тела)
WeekSelection decodeWeekSelection(Database& db, int sel);

const std::array<const char*, 6>& getDayNames();   // weekday 0..5
const std::array<const char*, 6>& getPairTimes();  // lesson_number 1..6 -> index 0..5

std::string formatDateLabel(const std::string& iso); // "YYYY-MM-DD" -> "DD-MM"
std::string makeISODateFromDayMonth(const std::string& ddmm, int year);

int chooseWeekOfCycleOrDate(Database& db);

// Утилиты безопасного ввода
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

// Если у тебя реально есть реализация этой функции где-то
void showGroupSchedule(Database& db, int groupId);
