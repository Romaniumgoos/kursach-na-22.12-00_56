#include "student.h"
#include "services/student_service.h"
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

    StudentService svc(db);

    while (true) {
        std::cout << "\n[STUDENT MENU]\n";
        std::cout << "1. Мои оценки по предметам\n";
        std::cout << "2. Посмотреть мою стипендию за сессию\n";
        std::cout << "3. Посмотреть мои пропуски\n";
        std::cout << "4. Моё расписание\n";
        std::cout << "0. Выход в авторизацию\n";

        int choice = readChoiceFromList("Ваш выбор", 0, 4, false, 0);
        if (choice == 0) break;

        switch (choice) {
        case 1: { // Мои оценки по предметам (семестр фиксирован = 1)
            int semesterId = 1;

            auto gradesRes = svc.getGradesForSemester(getId(), semesterId);
            if (!gradesRes.ok) {
                std::cout << "Ошибка при получении оценок: " << gradesRes.error << "\n";
                break;
            }

            const auto& grades = gradesRes.value;

            if (grades.empty()) {
                std::cout << "Оценок не найдено.\n";
                break;
            }

            // subject -> список оценок
            std::map<std::string, std::vector<int>> subjectMarks;
            for (const auto& g : grades) {
                std::string subj, date, gradeType;
                int value = 0;
                std::tie(subj, value, date, gradeType) = g;
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

                double avgSubj = marks.empty() ? 0.0 : static_cast<double>(sum) / marks.size();
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

        case 3:
            viewMyAbsences(db);
            break;

        case 4:
            viewMySchedule(db);
            break;

        default:
            std::cout << "Неверный пункт меню.\n";
            break;
        }
    }
}

// === МОЁ РАСПИСАНИЕ ===
bool Student::viewMySchedule(Database& db) {
    StudentService svc(db);

    // Если по какой-то причине ещё не загрузили (хотя menu обычно делает это)
    if (groupId == 0) {
        if (!loadGroupInfo(db)) {
            return false;
        }
    }

    const int sel = chooseWeekOfCycleOrDate(db);
    if (sel == 0) return false;

    const WeekSelection ws = decodeWeekSelection(db, sel);
    if (ws.weekOfCycle <= 0) {
        std::cout << "Некорректная неделя.\n";
        return false;
    }
    const int weekOfCycle = ws.weekOfCycle;

    const auto& dayNames  = getDayNames();
    const auto& pairTimes = getPairTimes();

    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║ МОЁ РАСПИСАНИЕ | Группа " << groupId
              << ", подгр. " << subgroup
              << " | неделя " << weekOfCycle << "\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";

    bool hasAnyLessons = false;

    for (int weekday = 0; weekday <= 5; ++weekday) {
        const auto rowsRes = svc.getScheduleForGroup(groupId, weekday, weekOfCycle);

        if (rowsRes.ok) {
            std::cerr << "[StudentSchedule self-test] weekOfCycle=" << weekOfCycle
                      << " weekday=" << weekday
                      << " rows=" << rowsRes.value.size() << "\n";
        }

        std::string dateLabel;
        const auto dateRes = svc.getDateISO(weekOfCycle, weekday);
        if (dateRes.ok) {
            dateLabel = formatDateLabel(dateRes.value);
        }

        std::cout << "\n" << dayNames[weekday];
        if (!dateLabel.empty()) std::cout << " " << dateLabel;
        std::cout << "\n";

        bool hasDayLessons = false;
        if (!rowsRes.ok || rowsRes.value.empty()) {
            std::cout << "  Занятий нет.\n";
            continue;
        }

        for (const auto& row : rowsRes.value) {
            auto [id, lessonNumber, rowSubgroup, subject, room, lessonType, teacher] = row;

            // студент видит общие (subgroup=0) и свои (subgroup совпадает)
            if (subgroup != 0 && rowSubgroup != 0 && rowSubgroup != subgroup) {
                continue;
            }

            hasAnyLessons = true;
            hasDayLessons = true;

            std::string subjectText = subject;
            if (rowSubgroup == 1)      subjectText += " (подгр. 1)";
            else if (rowSubgroup == 2) subjectText += " (подгр. 2)";

            std::string typeLabel;
            if (!lessonType.empty()) typeLabel = " [" + lessonType + "]";

            const std::string time = (lessonNumber >= 1 && lessonNumber <= 6) ? pairTimes[lessonNumber - 1] : "";

            std::cout << "  - " << subjectText << typeLabel << "\n";
            if (!time.empty() || !room.empty()) {
                std::cout << "    ";
                if (!time.empty()) std::cout << time;
                if (!time.empty() && !room.empty()) std::cout << " | ";
                if (!room.empty()) std::cout << "ауд. " << room;
                std::cout << "\n";
            }
            if (!teacher.empty()) {
                std::cout << "    " << teacher << "\n";
            }
        }

        if (!hasDayLessons) {
            std::cout << "  Занятий нет.\n";
        }
    }

    if (!hasAnyLessons) {
        std::cout << "На выбранной неделе для вашей подгруппы нет занятий.\n";
    }

    return true;
}

// === МОИ ПРОПУСКИ ===
void Student::viewMyAbsences(Database& db) {
    StudentService svc(db);
    const int semesterId = 1;

    const auto absRes = svc.getAbsencesForSemester(getId(), semesterId);
    if (!absRes.ok) {
        std::cout << "✗ Ошибка при получении данных: " << absRes.error << "\n";
        return;
    }
    if (absRes.value.empty()) {
        std::cout << "Пропусков за семестр не найдено.\n";
        return;
    }

    std::cout << "\n=== Мои пропуски за семестр ===\n";
    std::cout << std::left << std::setw(12) << "Дата"
              << std::setw(25) << "Предмет"
              << std::setw(8)  << "Часы"
              << std::setw(18) << "Тип" << "\n";
    std::cout << std::string(63, '-') << "\n";


    int total = 0;
    for (const auto& a : absRes.value) {
        const auto& [subject, hours, dateISO, type] = a;

        total += hours;

        std::cout << std::left << std::setw(12) << dateISO
                  << std::setw(25) << subject
                  << std::setw(8)  << hours
                  << std::setw(18) << (type == "excused" ? "уважительная" : "неуважительная")
                  << "\n";
    }

    std::cout << std::string(63, '-') << "\n";
    std::cout << "Всего часов пропусков: " << total << "\n";
}

// === Загрузка группы/подгруппы ===
bool Student::loadGroupInfo(Database& db) {
    StudentService svc(db);

    const auto gsRes = svc.getStudentGroupAndSubgroup(getId());
    if (!gsRes.ok) {
        std::cout << "[!] Не удалось загрузить группу/подгруппу: " << gsRes.error << "\n";
        groupId = 0;
        subgroup = 0;
        return false;
    }

    groupId = gsRes.value.first;
    subgroup = gsRes.value.second;
    return true;
}


