#ifndef MENU_UTILS_H
#define MENU_UTILS_H

#include <string>

// Функция для ввода целого числа в диапазоне
int readIntInRange(const std::string& prompt, int min, int max,
                   int defaultValue = -1, bool allowCancel = false,
                   int cancelValue = -1);

// Функция для выбора из списка
int readChoiceFromList(const std::string& prompt, int minChoice, int maxChoice,
                       bool allowCancel = false, int cancelValue = -1);

// Функция для ввода короткой строки (логин, номер аудитории и т.д.)
std::string readToken(const std::string& prompt, bool allowEmpty = false);

// Вспомогательная функция для очистки буфера ввода
void clearInputBuffer();
// Возвращает массив названий дней недели (0-пн, 1-вт, ..., 5-сб)
const std::array<const char*, 6>& getDayNames();

// Возвращает массив времен пар (0-первая пара, 1-вторая и т.д.)
const std::array<const char*, 6>& getPairTimes();

// Форматирует дату из формата YYYY-MM-DD в DD-MM
// Пример: "2025-12-22" -> "22-12"
std::string formatDateLabel(const std::string& isoDate);
#endif // MENU_UTILS_H
#pragma once
#include "database.h"

std::string makeISODateFromDayMonth(const std::string& ddmm, int year);

void showGroupSchedule(Database& db, int groupId);
int chooseWeekOfCycleOrDate(Database& db);
