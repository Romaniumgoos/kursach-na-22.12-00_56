#include "ui/widgets/PeriodSelectorWidget.h"
#include "database.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDate>
#include <sqlite3.h>

PeriodSelectorWidget::PeriodSelectorWidget(Database* db, QWidget* parent)
    : QWidget(parent), db(db)
{
    setupLayout();
    populateCalendarWeeks();

    // По умолчанию режим "Неделя цикла"
    modeCombo->setCurrentIndex(0);
    cycleWeekSpin->setValue(1);
    m_currentSelection = WeekSelection(1);

    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PeriodSelectorWidget::onModeChanged);
    connect(cycleWeekSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PeriodSelectorWidget::onCycleWeekChanged);
    connect(calendarWeekCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PeriodSelectorWidget::onCalendarWeekChanged);
    connect(dateEdit, &QDateEdit::dateChanged,
            this, &PeriodSelectorWidget::onDateChanged);
}

void PeriodSelectorWidget::setupLayout()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel("Период:"));

    modeCombo = new QComboBox(this);
    modeCombo->addItem("Неделя цикла (1-4)");
    modeCombo->addItem("Календарная неделя");
    modeCombo->addItem("По дате");
    layout->addWidget(modeCombo);

    cycleWeekSpin = new QSpinBox(this);
    cycleWeekSpin->setMinimum(1);
    cycleWeekSpin->setMaximum(4);
    cycleWeekSpin->setMinimumWidth(60);
    layout->addWidget(cycleWeekSpin);

    calendarWeekCombo = new QComboBox(this);
    calendarWeekCombo->setMinimumWidth(200);
    layout->addWidget(calendarWeekCombo);

    dateEdit = new QDateEdit(this);
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setMinimumWidth(120);
    layout->addWidget(dateEdit);

    layout->addStretch();

    // По умолчанию показываем только cycleWeekSpin_
    calendarWeekCombo->setVisible(false);
    dateEdit->setVisible(false);
}

void PeriodSelectorWidget::populateCalendarWeeks()
{
    if (!db) return;

    calendarWeekCombo->clear();

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, weekofcycle, startdate, enddate FROM cycleweeks ORDER BY id";

    if (sqlite3_prepare_v2(db->getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int weekNum = sqlite3_column_int(stmt, 1);
        const char* startText = (const char*)sqlite3_column_text(stmt, 2);
        const char* endText = (const char*)sqlite3_column_text(stmt, 3);

        QString display = QString::asprintf("Неделя %d (%s — %s)", weekNum, startText, endText);
        calendarWeekCombo->addItem(display, id);
    }

    sqlite3_finalize(stmt);
}

WeekSelection PeriodSelectorWidget::currentSelection() const
{
    return m_currentSelection;
}

void PeriodSelectorWidget::onModeChanged(int index)
{
    cycleWeekSpin->setVisible(index == 0);
    calendarWeekCombo->setVisible(index == 1);
    dateEdit->setVisible(index == 2);

    emitSelectionChanged();
}

void PeriodSelectorWidget::onCycleWeekChanged(int value)
{
    m_currentSelection = WeekSelection(value);
    emitSelectionChanged();
}

void PeriodSelectorWidget::onCalendarWeekChanged(int)
{
    int id = calendarWeekCombo->currentData().toInt();
    m_currentSelection = WeekSelection(WeekSelection::CalendarWeek, id);
    emitSelectionChanged();
}

void PeriodSelectorWidget::onDateChanged(const QDate& date)
{
    m_currentSelection = WeekSelection(WeekSelection::ByDate, date.toString("yyyy-MM-dd"));
    emitSelectionChanged();
}

void PeriodSelectorWidget::emitSelectionChanged()
{
    emit selectionChanged(m_currentSelection);
}
