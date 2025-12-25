#include "ui/pages/StudentSchedulePage.h"
#include "database.h"
#include "ui/util/UiStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <sqlite3.h>
#include <QHeaderView>

static void addDayHeaderRow(QTableWidget* table, const QString& dayTitle)
{
    const int row = table->rowCount();
    table->insertRow(row);

    table->setSpan(row, 0, 1, table->columnCount());
    table->setRowHeight(row, 44);

    // Колонка 0: текст дня, остальные пустые
    auto* item0 = new QTableWidgetItem(dayTitle);
    item0->setFlags(Qt::ItemIsEnabled); // не редактируется
    item0->setBackground(QColor(230, 235, 245));
    item0->setForeground(QColor(30, 30, 30));
    item0->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont f = item0->font();
    f.setBold(true);
    item0->setFont(f);

    table->setItem(row, 0, item0);

    // Заполняем пустыми ячейками остальные колонки, чтобы корректно красилось/выделялось
    for (int col = 1; col < table->columnCount(); ++col) {
        auto* it = new QTableWidgetItem("");
        it->setFlags(Qt::ItemIsEnabled);
        it->setBackground(QColor(230, 235, 245));
        table->setItem(row, col, it);
    }

    // Визуально делаем “объединение”: прячем вертикальные линии (у нас showGrid=false),
    // поэтому достаточно оставить текст в первой колонке.
}

static QColor colorForLessonType(const QString& lessonType)
{
    if (lessonType.contains("ЛР")) return QColor(255, 220, 220);
    if (lessonType.contains("ЛК")) return QColor(220, 255, 220);
    if (lessonType.contains("ПЗ")) return QColor(255, 250, 210);
    if (lessonType.contains("ЭКЗ") || lessonType.contains("экз", Qt::CaseInsensitive)) return QColor(255, 210, 210);
    return QColor(225, 225, 225);
}

static void addLessonRow(QTableWidget* table,
                         int lessonNum,
                         int rowSubgroup,
                         const QString& subject,
                         const QString& room,
                         const QString& lessonType,
                         const QString& teacher)
{
    QString subjectShown = subject;
    if (rowSubgroup == 1) subjectShown += "  [ПГ1]";
    else if (rowSubgroup == 2) subjectShown += "  [ПГ2]";

    const int tableRow = table->rowCount();
    table->insertRow(tableRow);

    // Узкая цветная полоса слева — тип пары
    auto* stripeItem = new QTableWidgetItem("");
    stripeItem->setFlags(Qt::ItemIsEnabled);
    stripeItem->setBackground(colorForLessonType(lessonType));
    table->setItem(tableRow, 0, stripeItem);

    auto* pairItem = new QTableWidgetItem(QString::number(lessonNum));
    pairItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(tableRow, 1, pairItem);

    table->setItem(tableRow, 2, new QTableWidgetItem(subjectShown));

    auto* typeItem = new QTableWidgetItem(lessonType);
    typeItem->setTextAlignment(Qt::AlignCenter);
    typeItem->setBackground(colorForLessonType(lessonType));
    if (rowSubgroup != 0) {
        typeItem->setToolTip(QString("Подгрупповая пара (ПГ%1)").arg(rowSubgroup));
    }
    table->setItem(tableRow, 3, typeItem);

    table->setItem(tableRow, 4, new QTableWidgetItem(room));
    table->setItem(tableRow, 5, new QTableWidgetItem(teacher));
}


StudentSchedulePage::StudentSchedulePage(Database* db, int studentId, QWidget* parent)
    : QWidget(parent), db(db), studentId(studentId), currentSubgroup(0)
{
    setupLayout();
    setupTable();
}

void StudentSchedulePage::setupLayout()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Панель выбора подгруппы
    auto* subgroupLayout = new QHBoxLayout();
    subgroupLayout->addWidget(new QLabel("Подгруппа:"));

    subgroupCombo = new QComboBox(this);
    subgroupCombo->addItem("Все пары", 0);
    subgroupCombo->addItem("Подгруппа 1", 1);
    subgroupCombo->addItem("Подгруппа 2", 2);
    subgroupCombo->setMaximumWidth(150);
    subgroupLayout->addWidget(subgroupCombo);
    subgroupLayout->addStretch();

    periodLabel = new QLabel("Период: —", this);
    periodLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
    subgroupLayout->addWidget(periodLabel);

    connect(subgroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        onSubgroupChanged(subgroupCombo->currentData().toInt());
    });

    mainLayout->addLayout(subgroupLayout);

    // Таблица
    table = new QTableWidget(this);
    mainLayout->addWidget(table);

    // Empty state
    emptyStateLabel = new QLabel("Нет занятий\nПопробуйте выбрать другую неделю или подгруппу", this);
    emptyStateLabel->setAlignment(Qt::AlignCenter);
    emptyStateLabel->setStyleSheet("color: palette(mid); font-size: 14px;");
    emptyStateLabel->setVisible(false);
    mainLayout->addWidget(emptyStateLabel);
}

void StudentSchedulePage::setupTable()
{
    UiStyle::applyStandardTableStyle(table);

    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"", "Пара", "Предмет", "Тип", "Аудитория", "Преподаватель"});
    auto* h = table->horizontalHeader();
    h->setSectionResizeMode(QHeaderView::Fixed);

    table->setColumnWidth(0, 12);  // Цветная полоса типа
    table->setColumnWidth(1, 55);  // Пара
    table->setColumnWidth(2, 220); // Предмет
    table->setColumnWidth(3, 70);  // Тип
    table->setColumnWidth(4, 90);  // Аудитория
    table->setColumnWidth(5, 180); // Преподаватель

    // table_->setColumnWidth(0, 80);
    // table_->setColumnWidth(1, 60);
    // table_->setColumnWidth(2, 150);
    // table_->setColumnWidth(3, 80);
    // table_->setColumnWidth(4, 80);
    // table_->setColumnWidth(5, 150);
    // table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    // table_->setSelectionMode(QAbstractItemView::SingleSelection);
}

void StudentSchedulePage::onPeriodChanged(const WeekSelection& selection)
{
    currentSelection = selection;
    loadSchedule();
}

void StudentSchedulePage::onSubgroupChanged(int subgroup)
{
    currentSubgroup = subgroup;
    loadSchedule();
}

void StudentSchedulePage::loadSchedule()
{
    if (!db) return;

    table->setRowCount(0);

    emptyStateLabel->setText("Нет занятий\nПопробуйте выбрать другую неделю или подгруппу");
    emptyStateLabel->setVisible(false);
    table->setVisible(true);

    // 1) weekOfCycle из выбранного периода


    // Всегда держим оба значения:
    // - resolvedWeekId: конкретная календарная неделя из cycleweeks (если удалось определить)
    // - weekOfCycle: 1..4 для выборки schedule
    int resolvedWeekId = 0;
    int weekOfCycle = 1;

    if (currentSelection.mode == WeekSelection::CycleWeek) {
        weekOfCycle = currentSelection.weekOfCycle;

        // Если хочется иметь дату в шапке даже в режиме CycleWeek — можно определить weekId по "сегодня"
        // но пока оставим 0 (шапка будет без даты).
        resolvedWeekId = 0;
    }
    else if (currentSelection.mode == WeekSelection::CalendarWeek) {
        resolvedWeekId = currentSelection.weekId;

        if (resolvedWeekId == 0) {
            table->setRowCount(0);
            table->setVisible(false);
            emptyStateLabel->setText("Неделя не найдена (weekId=0). Проверьте выбор периода.");
            emptyStateLabel->setVisible(true);
            return;
        }

        weekOfCycle = db->getWeekOfCycleByWeekId(resolvedWeekId);
        if (weekOfCycle == 0) {
            table->setRowCount(0);
            table->setVisible(false);
            emptyStateLabel->setText("Неделя не найдена в cycleweeks. Проверьте заполнение таблицы недель.");
            emptyStateLabel->setVisible(true);
            return;
        }
    }
    else if (currentSelection.mode == WeekSelection::ByDate) {
        // 1) Пытаемся найти конкретную календарную неделю (cycleweeks.id) по дате
        resolvedWeekId = db->getWeekIdByDate(currentSelection.selectedDate.toStdString());

        // ВАЖНО: если дата не покрывается таблицей cycleweeks — не показываем расписание по "левому" циклу.
        if (resolvedWeekId <= 0) {
            std::string minISO;
            std::string maxISO;
            {
                sqlite3_stmt* stmt = nullptr;
                const char* sql = "SELECT MIN(startdate), MAX(enddate) FROM cycleweeks;";
                if (sqlite3_prepare_v2(db->getHandle(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char* s1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                        const char* s2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                        if (s1) minISO = s1;
                        if (s2) maxISO = s2;
                    }
                }
                sqlite3_finalize(stmt);
            }

            table->setRowCount(0);
            table->setVisible(false);

            QString msg = "Для выбранной даты нет данных о цикле недель (cycleweeks).";
            if (!minISO.empty() && !maxISO.empty()) {
                msg += QString("\nВыберите дату в диапазоне %1 — %2")
                           .arg(QString::fromStdString(minISO))
                           .arg(QString::fromStdString(maxISO));
            }
            emptyStateLabel->setText(msg);
            emptyStateLabel->setVisible(true);
            return;
        }

        // Если нашли weekId — получаем weekOfCycle из этой недели
        weekOfCycle = db->getWeekOfCycleByWeekId(resolvedWeekId);
        if (weekOfCycle <= 0) weekOfCycle = 1;
    }


    // 2) Группа/подгруппа студента (чтобы не хранить groupId_ в UI)
    int groupId = 0;
    int studentSubgroup = 0;
    if (!db->getStudentGroupAndSubgroup(studentId, groupId, studentSubgroup)) {
        table->setVisible(false);
        emptyStateLabel->setText("Нет занятий\nПопробуйте выбрать другую неделю или подгруппу");
        emptyStateLabel->setVisible(true);
        return;
    }

    // Один раз при первом показе подставляем подгруппу студента по умолчанию,
    // чтобы расписание не выглядело пустым из-за фильтра.
    if (!subgroupAutoSelected && currentSubgroup == 0 && (studentSubgroup == 1 || studentSubgroup == 2)) {
        subgroupAutoSelected = true;
        if (subgroupCombo) {
            const int idx = subgroupCombo->findData(studentSubgroup);
            if (idx >= 0) {
                subgroupCombo->setCurrentIndex(idx);
                return;
            }
        }
        currentSubgroup = studentSubgroup;
    }

    if (periodLabel) {
        QString p = "Период: ";
        if (currentSelection.mode == WeekSelection::CycleWeek) {
            p += QString("Неделя цикла %1").arg(weekOfCycle);
        } else if (currentSelection.mode == WeekSelection::CalendarWeek) {
            p += QString("weekId %1 (цикл %2)").arg(resolvedWeekId).arg(weekOfCycle);
        } else if (currentSelection.mode == WeekSelection::ByDate) {
            p += QString("%1 (цикл %2)").arg(currentSelection.selectedDate).arg(weekOfCycle);
        }
        periodLabel->setText(p);
    }

#ifdef QT_DEBUG
    const char* modeStr = "Unknown";
    if (currentSelection.mode == WeekSelection::CycleWeek) modeStr = "CycleWeek";
    else if (currentSelection.mode == WeekSelection::CalendarWeek) modeStr = "CalendarWeek";
    else if (currentSelection.mode == WeekSelection::ByDate) modeStr = "ByDate";
    qDebug() << "[StudentSchedulePage] mode=" << modeStr
             << "resolvedWeekId=" << resolvedWeekId
             << "weekOfCycle=" << weekOfCycle
             << "groupId=" << groupId;
#endif

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
            "08:30-09:55", "10:05-11:30", "12:00-13:25",
            "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    int totalRows = 0;

    // 3) Идём по дням
    for (int weekday = 0; weekday <= 5; ++weekday) {

        bool dayHeaderAdded = false; // <-- ПЕРЕНЁС СЮДА

        std::vector<std::tuple<int,int,int,std::string,std::string,std::string,std::string>> rows;
        if (!db->getScheduleForGroup(groupId, weekday, weekOfCycle, rows)) {
            continue;
        }

        for (const auto& r : rows) {

            const int lessonNum = std::get<1>(r);
            const int rowSubgroup = std::get<2>(r);

            QString subject = QString::fromStdString(std::get<3>(r));
            QString room = QString::fromStdString(std::get<4>(r));
            QString lessonType = QString::fromStdString(std::get<5>(r));
            QString teacher = QString::fromStdString(std::get<6>(r));

    // Фильтр подгруппы:
    // - если currentSubgroup == 0: показываем всё
    // - иначе: показываем общие (0) + выбранную подгруппу
    if (currentSubgroup != 0) {
        if (rowSubgroup != 0 && rowSubgroup != currentSubgroup) continue;
    }

    Q_UNUSED(studentSubgroup);

            // ===== ШАПКА ДНЯ (один раз на день) =====
            if (!dayHeaderAdded) {
                dayHeaderAdded = true;

                QString headerText = dayNames[weekday];
                std::string dateISO;

                if (resolvedWeekId > 0) {
                    db->getDateForWeekdayByWeekId(resolvedWeekId, weekday, dateISO);
                } else {
                    db->getDateForWeekday(weekOfCycle, weekday, dateISO);
                }

                if (!dateISO.empty()) {
                    const QString q = QString::fromStdString(dateISO);
                    if (q.size() >= 10) {
                        const QString ddmm = q.mid(8, 2) + "." + q.mid(5, 2);
                        headerText = headerText + "\n" + ddmm;
                    } else {
                        headerText = headerText + "\n" + q;
                    }
                }

                addDayHeaderRow(table, headerText);
            }

            // ===== строка пары =====
            addLessonRow(table, lessonNum, rowSubgroup, subject, room, lessonType, teacher);
            ++totalRows;
        }

    }

    const bool empty = (totalRows == 0);
    table->setVisible(!empty);
    emptyStateLabel->setVisible(empty);
}
