// menu.h
#pragma once
#include "database.h"
std::string makeISODateFromDayMonth(const std::string& ddmm, int year);

void showGroupSchedule(Database& db, int groupId);
int chooseWeekOfCycleOrDate(Database& db);
