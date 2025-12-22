#include "menu.h"
#include "database.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <limits>
#include <sstream>
#include <iomanip>

// ---------- Safe input helpers ----------

static bool readLineSafe(std::string& out)
{
    out.clear();
    if (!std::getline(std::cin >> std::ws, out)) {
        return false;
    }
    return true;
}

int readIntInRange(const std::string& prompt,
                   int min, int max,
                   int defaultValue,
                   bool allowCancel,
                   int cancelValue)
{
    while (true) {
        std::cout << prompt << " [" << min << ".." << max << ", Enter=" << defaultValue;
        if (allowCancel) std::cout << ", c=отмена";
        std::cout << "]: ";

        std::string line;
        if (!readLineSafe(line)) {
            return allowCancel ? cancelValue : defaultValue;
        }

        if (allowCancel && (line == "c" || line == "C" || line == "cancel" || line == "CANCEL")) {
            return cancelValue;
        }
        if (line.empty()) {
            return defaultValue;
        }

        std::istringstream iss(line);
        int v = 0;
        char extra = 0;

        if (!(iss >> v) || (iss >> extra)) {
            std::cout << "Некорректный ввод. Введите целое число.\n";
            continue;
        }

        if (v < min || v > max) {
            std::cout << "Вне диапазона (" << min << ".." << max << "). Повторите.\n";
            continue;
        }

        return v;
    }
}

int readChoiceFromList(const std::string& prompt,
                       int minChoice, int maxChoice,
                       bool allowCancel,
                       int cancelValue)
{
    return readIntInRange(prompt, minChoice, maxChoice, minChoice, allowCancel, cancelValue);
}

std::string readToken(const std::string& prompt)
{
    while (true) {
        std::cout << prompt << ": ";
        std::string line;
        if (!readLineSafe(line)) return {};
        if (line.empty()) {
            std::cout << "Пустой ввод. Повторите.\n";
            continue;
        }
        return line;
    }
}

// ---------- Existing menu logic ----------

// "DD-MM" + год → "YYYY-MM-DD"
std::string makeISODateFromDayMonth(const std::string& ddmm, int year)
{
    int d = 0, m = 0;
    char dash = '-';
    std::istringstream iss(ddmm);
    iss >> d >> dash >> m;
    if (!iss || dash != '-' || d < 1 || d > 31 || m < 1 || m > 12) {
        return {};
    }

    std::ostringstream oss;
    oss << year << '-'
        << std::setw(2) << std::setfill('0') << m << '-'
        << std::setw(2) << std::setfill('0') << d;
    return oss.str();
}

// единый выбор недели: номер / список / дата
int chooseWeekOfCycleOrDate(Database& db)
{
    std::cout << "\nКак выбрать неделю?\n";
    std::cout << "1. Ввести номер недели цикла (1-4)\n";
    std::cout << "2. Выбрать из списка календарных недель\n";
    std::cout << "3. Ввести конкретную дату (DD-MM)\n";

    int mode = readIntInRange("Ваш выбор", 1, 3, 1, true, 0);
    if (mode == 0) return 0;

    if (mode == 1) {
        int week = readIntInRange("Введите номер недели (1-4)", 1, 4, 1, true, 0);
        return week; // 0 = отмена
    }

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

        int idChoice = readIntInRange("Введите ID недели", 1, 1000000, 1, true, 0);
        if (idChoice == 0) return 0;

        for (const auto& w : weeks) {
            int id, weekCycle;
            std::string start, end;
            std::tie(id, weekCycle, start, end) = w;
            if (id == idChoice) return weekCycle;
        }

        std::cout << "Нет недели с таким ID.\n";
        return 0;
    }

    // mode == 3
    std::string ddmm = readToken("Введите дату (DD-MM)");
    std::string date = makeISODateFromDayMonth(ddmm, 2025);
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
