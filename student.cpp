#include "student.h"

#include "database.h"
#include "statistics.h"
#include "scholarship.h"
#include "menu.h"
#include "admin.h"

#include <iostream>
#include <vector>
#include <map>
#include <tuple>
#include <iomanip>
#include <sstream>

void Student::displayMenu(Database& db) {
    // загрузить группу и подгруппу перед первым показом меню
    loadGroupInfo(db);

    while (true) {
        std::cout << "\n[STUDENT MENU]\n";
        std::cout << "1. Мои оценки по предметам\n";
        std::cout << "2. Посмотреть мою стипендию за сессию\n";
        std::cout << "3. Посмотреть мои пропуски\n";
        std::cout << "4. Моё расписание\n";
        std::cout << "0. Выход в авторизацию\n";

        int choice = 0;
        std::cin >> choice;
        if (choice == 0) {
            break;
        }

        switch (choice) {
        case 1: { // Мои оценки по предметам (семестр фиксирован = 1)
            int semesterId = 1;
            std::vector<std::tuple<std::string, int, std::string, std::string>> grades;

            if (!db.getStudentGradesForSemester(getId(), semesterId, grades)) {
                std::cout << "Ошибка при получении оценок.\n";
                break;
            }

            if (grades.empty()) {
                std::cout << "Оценок не найдено.\n";
                break;
            }

            // subject -> список оценок
            std::map<std::string, std::vector<int>> subjectMarks;
            for (const auto& g : grades) {
                std::string subj, date, grade_type;
                int value;
                std::tie(subj, value, date, grade_type) = g;  // ← ИСПРАВЛЕНО: 4 элемента
                subjectMarks[subj].push_back(value);
            }

            double totalSum = 0.0;
            int totalCount = 0;

            std::cout << "\n=== Мои оценки по предметам ===\n";
            for (const auto& [subject, marks] : subjectMarks) {
                std::cout << "\nПредмет: " << subject << "\nОценки: ";
                int sum = 0;
                for (int v : marks) {
                    std::cout << v << " ";
                    sum += v;
                }
                double avgSubj = marks.empty() ? 0.0
                    : static_cast<double>(sum) / marks.size();
                std::cout << "\nСредний балл по предмету: " << avgSubj << "\n";

                totalSum += sum;
                totalCount += static_cast<int>(marks.size());
            }

            if (totalCount > 0) {
                double overall = totalSum / totalCount;
                std::cout << "\nОбщий средний балл по всем предметам: " << overall << "\n";
            }
            break;
        }

        case 2: { // Стипендия, семестр тоже фиксированный = 1
            int semesterId = 1;
            double avg = Statistics::calculateStudentAverage(db, getId(), semesterId);
            ScholarshipInfo info = ScholarshipCalculator::calculate(avg);

            int totalAbsencesHours = 0;
            db.getStudentTotalAbsences(getId(), semesterId, totalAbsencesHours);

            int unexcusedHours = 0;
            if (!db.getStudentUnexcusedAbsences(getId(), semesterId, unexcusedHours)) {
                std::cout << "[!] Не удалось получить данные о неуважительных пропусках.\n";
            }

            bool strippedByAbsences = false;
            if (unexcusedHours >= 30) {
                strippedByAbsences = true;
                info.coefficient = 0.0;
                info.amountBYN = 0.0;
            }

            std::cout << "\n=== Расчёт стипендии за семестр ===\n";
            std::cout << "Средний балл: " << avg << "\n";
            std::cout << "Суммарные пропуски за семестр: " << totalAbsencesHours << " часов\n";
            std::cout << "Из них неуважительные: " << unexcusedHours << " часов\n";

            if (info.coefficient <= 0.0) {
                if (strippedByAbsences) {
                    std::cout << "Стипендия не выплачивается (пропуски 30 часов и более за семестр).\n";
                } else {
                    std::cout << "Стипендия не выплачивается (средний балл ниже 5.0).\n";
                }
            } else {
                std::cout << "Коэффициент: " << info.coefficient << "\n";
                std::cout << "Размер стипендии: " << info.amountBYN << " BYN\n";
            }
            break;
        }

        case 3: {
            viewMyAbsences(db);
            break;
        }

        case 4: {
            viewMySchedule(db);
            break;
        }

        default:
            std::cout << "Неверный пункт меню.\n";
            break;
        }
    }
}

// === МОЁ РАСПИСАНИЕ ===
bool Student::viewMySchedule(Database& db) {
    int groupId = 0;
    int subgroup = 0;
    if (!db.getStudentGroupAndSubgroup(getId(), groupId, subgroup)) {
        std::cout << "Не удалось определить вашу группу/подгруппу.\n";
        return false;
    }

    int week = chooseWeekOfCycleOrDate(db);
    if (week == 0) return false;

    const char* dayNames[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
    const char* pairTimes[] = {
        "08:30-09:55",
        "10:05-11:30",
        "12:00-13:25",
        "13:35-15:00",
        "15:30-16:55",
        "17:05-18:30"
    };

    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║ МОЁ РАСПИСАНИЕ | Группа " << groupId
              << ", подгр. " << subgroup
              << " | неделя " << week << "\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";

    bool hasAnyLessons = false;

    for (int weekday = 0; weekday <= 5; ++weekday) {
        std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> rows;
        if (!db.getScheduleForGroup(groupId, weekday, week, rows)) {
            continue;
        }
        if (rows.empty()) continue;

        std::string dateISO;
        std::string dateLabel;
        if (db.getDateForWeekday(week, weekday, dateISO) && dateISO.size() == 10) {
            dateLabel = dateISO.substr(8, 2) + "-" + dateISO.substr(5, 2);
        }

        if (!dateLabel.empty()) {
            std::cout << "\n[" << dayNames[weekday] << "] (" << dateLabel << ")\n";
        } else {
            std::cout << "\n[" << dayNames[weekday] << "]\n";
        }

        for (const auto& row : rows) {
            int    id, lessonNumber, subgrp;
            std::string subj, room, ltype, teacher;
            std::tie(id, lessonNumber, subgrp, subj, room, ltype, teacher) = row;

            // фильтр по подгруппе: студент видит свои подгрупповые и общие
            if (subgroup != 0 && subgrp != 0 && subgrp != subgroup) {
                continue;
            }

            hasAnyLessons = true;

            std::string subjText = subj;
            if (subgrp == 1)      subjText += " (подгр. 1)";
            else if (subgrp == 2) subjText += " (подгр. 2)";

            std::string typeLabel;
            if (!ltype.empty()) {
                typeLabel = " [" + ltype + "]";
            }

            std::cout << "  Пара " << lessonNumber;
            if (lessonNumber >= 1 && lessonNumber <= 6) {
                std::cout << " (" << pairTimes[lessonNumber - 1] << ")";
            }

            std::cout << " | " << subjText << typeLabel
                      << " | преп. " << teacher
                      << " | ауд. " << room << "\n";
        }
    }

    if (!hasAnyLessons) {
        std::cout << "На выбранной неделе для вашей подгруппы нет занятий.\n";
    }

    return true;
}

// === МОИ ПРОПУСКИ ===
void Student::viewMyAbsences(Database& db) {
    int semesterId = 1;
    std::vector<std::tuple<std::string,int,std::string,std::string>> absences;
    if (!db.getStudentAbsencesForSemester(getId(), semesterId, absences)) {
        std::cout << "✗ Ошибка при получении данных.\n";
        return;
    }

    if (absences.empty()) {
        std::cout << "Пропусков за семестр не найдено.\n";
        return;
    }

    std::cout << "\n=== Мои пропуски за семестр ===\n";
    std::cout << std::left << std::setw(12) << "Дата"
              << std::setw(25) << "Предмет"
              << std::setw(8) << "Часы"
              << std::setw(18) << "Тип" << "\n";
    std::cout << std::string(63, '─') << "\n";

    int total = 0;
    for (const auto& a : absences) {
        const std::string& subj = std::get<0>(a);
        int hours               = std::get<1>(a);
        const std::string& date = std::get<2>(a);
        const std::string& type = std::get<3>(a);
        total += hours;

        std::cout << std::left << std::setw(12) << date
                  << std::setw(25) << subj
                  << std::setw(8) << hours
                  << std::setw(18) << (type == "excused" ? "уважительная" : "неуважительная")
                  << "\n";
    }

    std::cout << std::string(63, '─') << "\n";
    std::cout << "Всего часов пропусков: " << total << "\n";
}

// === Загрузка группы/подгруппы ===
bool Student::loadGroupInfo(Database& db) {
    int g = 0, sub = 0;
    if (!db.getStudentGroupAndSubgroup(getId(), g, sub)) {
        std::cout << "[!] Не удалось загрузить группу и подгруппу для студента.\n";
        groupId_ = 0;
        subgroup_ = 0;
        return false;
    }
    groupId_ = g;
    subgroup_ = sub;
    return true;
}
