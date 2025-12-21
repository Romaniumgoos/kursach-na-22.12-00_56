#include "menu.h"
#include "database.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <limits>
#include <sstream>
#include <iomanip>

// "DD-MM" + год → "YYYY-MM-DD"
std::string makeISODateFromDayMonth(const std::string& ddmm, int year) {
    int d = 0, m = 0;
    char dash = '-';
    std::istringstream iss(ddmm);
    iss >> d >> dash >> m;
    if (!iss || dash != '-' || d < 1 || d > 31 || m < 1 || m > 12) {
        return {}; // ошибка
    }

    std::ostringstream oss;
    oss << year << '-'
        << std::setw(2) << std::setfill('0') << m << '-'
        << std::setw(2) << std::setfill('0') << d;
    return oss.str();
}

// единый выбор недели: номер / список / дата
int chooseWeekOfCycleOrDate(Database& db) {
    std::cout << "\nКак выбрать неделю?\n";
    std::cout << "1. Ввести номер недели цикла (1-4)\n";
    std::cout << "2. Выбрать из списка календарных недель\n";
    std::cout << "3. Ввести конкретную дату (DD-MM)\n";
    std::cout << "Ваш выбор: ";
    int mode = 0;
    std::cin >> mode;

    // 1) Просто номер 1-4
    if (mode == 1) {
        int week = 1;
        std::cout << "Введите номер недели (1-4): ";
        std::cin >> week;

        if (!std::cin.good() || week < 1 || week > 4) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Некорректный номер, используем неделю 1.\n";
            week = 1;
        }
        return week;
    }

    // 2) Выбор из списка calendar weeks
    if (mode == 2) {
        std::vector<std::tuple<int,int,std::string,std::string>> weeks;
        if (!db.getCycleWeeks(weeks) || weeks.empty()) {
            std::cout << "Нет данных по неделям цикла.\n";
            return 0;
        }

        std::cout << "\nСписок календарных недель:\n";
        for (const auto& w : weeks) {
            int id, weekCycle;
            std::string start, end;
            std::tie(id, weekCycle, start, end) = w;
            std::cout << id << ". Неделя цикла " << weekCycle
                      << " (" << start << " — " << end << ")\n";
        }

        std::cout << "Введите ID недели: ";
        int idChoice = 0;
        std::cin >> idChoice;
        if (!std::cin.good()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Некорректный ввод.\n";
            return 0;
        }

        for (const auto& w : weeks) {
            int id, weekCycle;
            std::string start, end;
            std::tie(id, weekCycle, start, end) = w;
            if (id == idChoice) {
                return weekCycle;
            }
        }

        std::cout << "Нет недели с таким ID.\n";
        return 0;
    }

    // 3) Ввод даты DD-MM
    if (mode == 3) {
        std::string ddmm;
        std::cout << "Введите дату (DD-MM): ";
        std::cin >> ddmm;

        std::string date = makeISODateFromDayMonth(ddmm, 2025); // учебный год
        if (date.empty()) {
            std::cout << "Некорректная дата.\n";
            return 0;
        }

        int w = db.getWeekOfCycleByDate(date);
        if (w == 0) {
            std::cout << "Эта дата не входит ни в одну учебную неделю.\n";
            return 0;
        }

        std::cout << "Эта дата попадает в неделю цикла: " << w << "\n";
        return w;
    }

    std::cout << "Некорректный выбор.\n";
    return 0;
}
