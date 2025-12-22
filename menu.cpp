#include "menu.h"
#include "database.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <limits>
#include <sstream>
#include <iomanip>
#include <string>

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readIntInRange(const std::string& prompt, int min, int max,
                   int defaultValue, bool allowCancel, int cancelValue) {
    int value;
    while (true) {
        std::cout << prompt;
        if (defaultValue != -1) {
            std::cout << " [" << defaultValue << "]: ";
        } else {
            std::cout << " ";
        }

        std::string input;
        std::getline(std::cin, input);

        // Обработка пустого ввода
        if (input.empty() && defaultValue != -1) {
            return defaultValue;
        }

        // Обработка отмены
        if (allowCancel && (input == "q" || input == "Q" || input == "отмена")) {
            return cancelValue;
        }

        try {
            size_t pos;
            value = std::stoi(input, &pos);

            // Проверяем, что ввели только число
            if (pos != input.length()) {
                throw std::invalid_argument("Некорректный ввод");
            }

            if (value >= min && value <= max) {
                return value;
            } else {
                std::cout << "Ошибка: введите число от " << min << " до " << max;
                if (allowCancel) {
                    std::cout << " или 'q' для отмены";
                }
                std::cout << std::endl;
            }
        } catch (const std::exception&) {
            std::cout << "Ошибка: введите корректное число";
            if (allowCancel) {
                std::cout << " или 'q' для отмены";
            }
            std::cout << std::endl;
        }
    }
}

int readChoiceFromList(const std::string& prompt, int minChoice, int maxChoice,
                       bool allowCancel, int cancelValue) {
    return readIntInRange(prompt, minChoice, maxChoice, -1, allowCancel, cancelValue);
}

std::string readToken(const std::string& prompt, bool allowEmpty) {
    std::string input;
    while (true) {
        std::cout << prompt << ": ";
        std::getline(std::cin, input);

        if (input.empty() && !allowEmpty) {
            std::cout << "Ошибка: ввод не может быть пустым" << std::endl;
            continue;
        }

        // Удаляем лишние пробелы в начале и конце
        input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
        input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);

        // Проверяем на наличие запрещенных символов
        if (input.find(';') != std::string::npos ||
            input.find('"') != std::string::npos) {
            std::cout << "Ошибка: ввод содержит недопустимые символы" << std::endl;
            continue;
        }

        return input;
    }
}
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
//
// const std::array<const char*, 6>& getDayNames() {
//     static const std::array<const char*, 6> days = {
//         "Понедельник", "Вторник", "Среда",
//         "Четверг", "Пятница", "Суббота"
//     };
//     return days;
// }
//
// const std::array<const char*, 6>& getPairTimes() {
//     static const std::array<const char*, 6> times = {
//         "9:00 - 10:30",  // 1 пара
//         "10:40 - 12:10", // 2 пара
//         "12:20 - 13:50", // 3 пара
//         "14:30 - 16:00", // 4 пара
//         "16:10 - 17:40", // 5 пара
//         "17:50 - 19:20"  // 6 пара
//     };
//     return times;
// }
//
// std::string formatDateLabel(const std::string& isoDate) {
//     if (isoDate.length() < 10) return isoDate;
//     // Формат: DD-MM
//     return isoDate.substr(8, 2) + "-" + isoDate.substr(5, 2);
// }
