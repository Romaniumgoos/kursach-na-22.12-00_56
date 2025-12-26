#include "ui/widgets/WeekGridScheduleWidget.h"

#include "database.h"
#include "ui/widgets/LessonCardWidget.h"

#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDate>
#include <QStyle>

WeekGridScheduleWidget::WeekGridScheduleWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("WeekGridScheduleWidget");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setObjectName("WeekGridScrollArea");

    contentWidget = new QWidget(scrollArea);
    contentWidget->setObjectName("WeekGridContent");
    grid = new QGridLayout(contentWidget);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);

    scrollArea->setWidget(contentWidget);
    root->addWidget(scrollArea);
}

void WeekGridScheduleWidget::repolish(QWidget* w)
{
    if (!w) return;
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void WeekGridScheduleWidget::clearGrid()
{
    if (!grid) return;

    selectedLessonCardBody = nullptr;

    while (QLayoutItem* item = grid->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

bool WeekGridScheduleWidget::isRowVisibleForSubgroup(int rowSubgroup, int selectedSubgroup)
{
    if (selectedSubgroup == 0) return true;
    if (rowSubgroup == 0) return true;
    return rowSubgroup == selectedSubgroup;
}

QString WeekGridScheduleWidget::dateISOForDay(Database* db, int weekOfCycle, int resolvedWeekId, int weekday)
{
    if (!db) return {};

    std::string iso;
    if (resolvedWeekId > 0) {
        db->getDateForWeekdayByWeekId(resolvedWeekId, weekday, iso);
    } else {
        db->getDateForWeekday(weekOfCycle, weekday, iso);
    }

    return QString::fromStdString(iso);
}

QString WeekGridScheduleWidget::dayHeaderText(const QString& dayName, const QString& dateISO)
{
    QString ddmm;
    QString suffix;

    if (!dateISO.isEmpty() && dateISO.size() >= 10) {
        ddmm = dateISO.mid(8, 2) + "." + dateISO.mid(5, 2);

        const QDate d = QDate::fromString(dateISO.left(10), "yyyy-MM-dd");
        if (d.isValid()) {
            const QDate today = QDate::currentDate();
            if (d == today) suffix = " (сегодня)";
            else if (d == today.addDays(1)) suffix = " (завтра)";
        }
    }

    QString line2 = ddmm;
    if (!suffix.isEmpty()) {
        if (!line2.isEmpty()) line2 += suffix;
        else line2 = suffix.trimmed();
    }

    if (!line2.isEmpty()) return dayName + "\n" + line2;
    return dayName;
}

void WeekGridScheduleWidget::setSchedule(Database* db,
                                        int groupId,
                                        int weekOfCycle,
                                        int resolvedWeekId,
                                        int currentSubgroup)
{
    clearGrid();

    selectedLessonCardBody = nullptr;

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    // corner
    auto* corner = new QLabel("", contentWidget);
    corner->setFixedHeight(52);
    corner->setObjectName("WeekGridCorner");
    grid->addWidget(corner, 0, 0);

    // day headers (row 0, cols 1..6)
    for (int weekday = 1; weekday <= 6; ++weekday) {
        const QString iso = dateISOForDay(db, weekOfCycle, resolvedWeekId, weekday);
        auto* header = new QLabel(dayHeaderText(dayNames[weekday - 1], iso), contentWidget);
        header->setAlignment(Qt::AlignCenter);
        header->setFixedHeight(52);
        header->setObjectName("WeekGridDayHeader");
        header->setWordWrap(true);
        grid->addWidget(header, 0, weekday);
    }

    // time column + cells
    for (int lessonIndex = 0; lessonIndex < 6; ++lessonIndex) {
        auto* timeLabel = new QLabel(pairTimes[lessonIndex], contentWidget);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setObjectName("WeekGridTimeLabel");
        timeLabel->setFixedWidth(110);
        timeLabel->setMinimumHeight(90);
        grid->addWidget(timeLabel, 1 + lessonIndex, 0);

        for (int weekday = 1; weekday <= 6; ++weekday) {
            auto* cell = new QWidget(contentWidget);
            cell->setObjectName("WeekGridCell");

            auto* v = new QVBoxLayout(cell);
            v->setContentsMargins(8, 8, 8, 8);
            v->setSpacing(8);

            // gather lessons for this cell
            std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> rows;
            bool ok = false;
            if (db) {
                ok = db->getScheduleForGroup(groupId, weekday, weekOfCycle, rows);
            }

            const int lessonNum = lessonIndex + 1;
            int cardCount = 0;

            if (ok) {
                for (const auto& r : rows) {
                    const int scheduleId = std::get<0>(r);
                    const int rowLessonNum = std::get<1>(r);
                    if (rowLessonNum != lessonNum) continue;

                    const int rowSubgroup = std::get<2>(r);
                    if (!isRowVisibleForSubgroup(rowSubgroup, currentSubgroup)) continue;

                    const QString subject = QString::fromStdString(std::get<3>(r));
                    const QString room = QString::fromStdString(std::get<4>(r));
                    const QString lessonType = QString::fromStdString(std::get<5>(r));
                    const QString teacher = QString::fromStdString(std::get<6>(r));

                    auto* card = new LessonCardWidget(scheduleId, subject, room, lessonType, teacher, rowSubgroup, cell);
                    connect(card, &LessonCardWidget::clicked, this, [this, card](int id) {
                        QWidget* body = card->findChild<QWidget*>("LessonCardBody");
                        if (selectedLessonCardBody && selectedLessonCardBody != body) {
                            selectedLessonCardBody->setProperty("selected", false);
                            repolish(selectedLessonCardBody);
                        }
                        selectedLessonCardBody = body;
                        if (selectedLessonCardBody) {
                            selectedLessonCardBody->setProperty("selected", true);
                            repolish(selectedLessonCardBody);
                        }
                        emit lessonClicked(id);
                    });
                    v->addWidget(card);
                    ++cardCount;
                }
            }

            if (cardCount == 0) {
                auto* empty = new QLabel("Занятий нет", cell);
                empty->setAlignment(Qt::AlignCenter);
                empty->setObjectName("WeekGridEmptyLabel");
                v->addWidget(empty, 1);
            }

            v->addStretch(1);

            cell->setMinimumHeight(90);
            grid->addWidget(cell, 1 + lessonIndex, weekday);
        }
    }

    // make columns stretch nicely
    grid->setColumnStretch(0, 0);
    for (int c = 1; c <= 6; ++c) grid->setColumnStretch(c, 1);
}

void WeekGridScheduleWidget::setTeacherScheduleAllGroups(Database* db,
                                                        int teacherId,
                                                        int weekOfCycle,
                                                        int resolvedWeekId,
                                                        int currentSubgroup)
{
    clearGrid();

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string,std::string>> weekRows;
    bool ok = false;
    if (db) {
        ok = db->getScheduleForTeacherWeekWithRoom(teacherId, weekOfCycle, currentSubgroup, weekRows);
    }

    auto* corner = new QLabel("", contentWidget);
    corner->setFixedHeight(52);
    corner->setObjectName("WeekGridCorner");
    grid->addWidget(corner, 0, 0);

    for (int weekday = 1; weekday <= 6; ++weekday) {
        const QString iso = dateISOForDay(db, weekOfCycle, resolvedWeekId, weekday);
        auto* header = new QLabel(dayHeaderText(dayNames[weekday - 1], iso), contentWidget);
        header->setAlignment(Qt::AlignCenter);
        header->setFixedHeight(52);
        header->setObjectName("WeekGridDayHeader");
        header->setWordWrap(true);
        grid->addWidget(header, 0, weekday);
    }

    for (int lessonIndex = 0; lessonIndex < 6; ++lessonIndex) {
        auto* timeLabel = new QLabel(pairTimes[lessonIndex], contentWidget);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setObjectName("WeekGridTimeLabel");
        timeLabel->setFixedWidth(110);
        timeLabel->setMinimumHeight(90);
        grid->addWidget(timeLabel, 1 + lessonIndex, 0);

        const int lessonNum = lessonIndex + 1;

        for (int weekday = 1; weekday <= 6; ++weekday) {
            auto* cell = new QWidget(contentWidget);
            cell->setObjectName("WeekGridCell");

            auto* v = new QVBoxLayout(cell);
            v->setContentsMargins(8, 8, 8, 8);
            v->setSpacing(8);

            int cardCount = 0;
            if (ok) {
                for (const auto& r : weekRows) {
                    const int rowWeekday = std::get<2>(r);
                    const int rowLessonNum = std::get<3>(r);
                    if (rowWeekday != weekday) continue;
                    if (rowLessonNum != lessonNum) continue;

                    const int rowSubgroup = std::get<4>(r);
                    if (!isRowVisibleForSubgroup(rowSubgroup, currentSubgroup)) continue;

                    const QString subject = QString::fromStdString(std::get<5>(r));
                    const QString room = QString::fromStdString(std::get<6>(r));
                    const QString lessonType = QString::fromStdString(std::get<7>(r));

                    QString groupName = QString::fromStdString(std::get<8>(r));
                    const int groupId = std::get<1>(r);
                    if (groupName.isEmpty()) {
                        if (groupId == 0) groupName = "Общая";
                        else groupName = QString("Группа %1").arg(groupId);
                    }

                    v->addWidget(new LessonCardWidget(subject, room, lessonType, groupName, rowSubgroup, cell));
                    ++cardCount;
                }
            }

            if (cardCount == 0) {
                auto* empty = new QLabel("Занятий нет", cell);
                empty->setAlignment(Qt::AlignCenter);
                empty->setObjectName("WeekGridEmptyLabel");
                v->addWidget(empty, 1);
            }

            v->addStretch(1);
            cell->setMinimumHeight(90);
            grid->addWidget(cell, 1 + lessonIndex, weekday);
        }
    }

    grid->setColumnStretch(0, 0);
    for (int c = 1; c <= 6; ++c) grid->setColumnStretch(c, 1);
}

void WeekGridScheduleWidget::setTeacherSchedule(Database* db,
                                               int teacherId,
                                               int groupId,
                                               const QString& groupName,
                                               int weekOfCycle,
                                               int resolvedWeekId,
                                               int currentSubgroup)
{
    clearGrid();

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string>> weekRows;
    bool ok = false;
    if (db) {
        ok = db->getScheduleForTeacherGroupWeekWithRoom(teacherId, groupId, weekOfCycle, currentSubgroup, weekRows);
    }

    // corner
    auto* corner = new QLabel("", contentWidget);
    corner->setFixedHeight(52);
    corner->setObjectName("WeekGridCorner");
    grid->addWidget(corner, 0, 0);

    // day headers
    for (int weekday = 1; weekday <= 6; ++weekday) {
        const QString iso = dateISOForDay(db, weekOfCycle, resolvedWeekId, weekday);
        auto* header = new QLabel(dayHeaderText(dayNames[weekday - 1], iso), contentWidget);
        header->setAlignment(Qt::AlignCenter);
        header->setFixedHeight(52);
        header->setObjectName("WeekGridDayHeader");
        header->setWordWrap(true);
        grid->addWidget(header, 0, weekday);
    }

    for (int lessonIndex = 0; lessonIndex < 6; ++lessonIndex) {
        auto* timeLabel = new QLabel(pairTimes[lessonIndex], contentWidget);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setObjectName("WeekGridTimeLabel");
        timeLabel->setFixedWidth(110);
        timeLabel->setMinimumHeight(90);
        grid->addWidget(timeLabel, 1 + lessonIndex, 0);

        const int lessonNum = lessonIndex + 1;

        for (int weekday = 1; weekday <= 6; ++weekday) {
            auto* cell = new QWidget(contentWidget);
            cell->setObjectName("WeekGridCell");

            auto* v = new QVBoxLayout(cell);
            v->setContentsMargins(8, 8, 8, 8);
            v->setSpacing(8);

            int cardCount = 0;
            if (ok) {
                for (const auto& r : weekRows) {
                    const int rowWeekday = std::get<2>(r);
                    const int rowLessonNum = std::get<3>(r);
                    if (rowWeekday != weekday) continue;
                    if (rowLessonNum != lessonNum) continue;

                    const int rowSubgroup = std::get<4>(r);
                    if (!isRowVisibleForSubgroup(rowSubgroup, currentSubgroup)) continue;

                    const QString subject = QString::fromStdString(std::get<5>(r));
                    const QString room = QString::fromStdString(std::get<6>(r));
                    const QString lessonType = QString::fromStdString(std::get<7>(r));

                    // В Teacher-расписании вместо преподавателя показываем группу (контекст)
                    v->addWidget(new LessonCardWidget(subject, room, lessonType, groupName, rowSubgroup, cell));
                    ++cardCount;
                }
            }

            if (cardCount == 0) {
                auto* empty = new QLabel("Занятий нет", cell);
                empty->setAlignment(Qt::AlignCenter);
                empty->setObjectName("WeekGridEmptyLabel");
                v->addWidget(empty, 1);
            }

            v->addStretch(1);
            cell->setMinimumHeight(90);
            grid->addWidget(cell, 1 + lessonIndex, weekday);
        }
    }

    grid->setColumnStretch(0, 0);
    for (int c = 1; c <= 6; ++c) grid->setColumnStretch(c, 1);
}
