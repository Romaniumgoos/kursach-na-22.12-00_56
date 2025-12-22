
// menu.h
#pragma once
#include "database.h"
#include <string>
#include <array>


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
