#include <QApplication>
#include <QMessageBox>
#include "loginwindow.h"
#include "database.h"
#include "config.h"   // добавь вверху файла




int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Подключаемся к БД (файл рядом с .exe)
    Database db(PROJECT_ROOT + "\\school.db");

    if (!db.connect()) {
        QMessageBox::critical(nullptr,
                              "Ошибка БД",
                              "Не удалось подключиться к базе данных (school.db).");
        return 1;
    }

    // Если надо — инициализировать структуру таблиц
    if (!db.initialize()) {
        QMessageBox::critical(nullptr,
                              "Ошибка БД",
                              "Не удалось инициализировать структуру БД.");
        return 1;
    }

    // Проверяем, пустое ли расписание
    bool isEmpty = true;
    if (db.isScheduleEmpty(isEmpty) && isEmpty) {
        if (!db.initializeDemoData()) {
            QMessageBox::warning(nullptr,
                                 "Предупреждение",
                                 "Не удалось загрузить демо‑данные.");
        }

        // Загрузка расписаний для 4 групп из файлов в корне проекта
        db.loadGroupSchedule(1, PROJECT_ROOT + "\\schedule_420601_newest.sql");
        db.loadGroupSchedule(2, PROJECT_ROOT + "\\schedule_420602_newest.sql");
        db.loadGroupSchedule(3, PROJECT_ROOT + "\\schedule_420603_newest.sql");
        db.loadGroupSchedule(4, PROJECT_ROOT + "\\schedule_420604_newest.sql");

    }

    // Показываем окно логина
    LoginWindow w(&db);
    w.show();

    return app.exec();
}
