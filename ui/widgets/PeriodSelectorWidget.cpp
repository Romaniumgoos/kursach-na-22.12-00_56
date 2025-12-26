#include "ui/widgets/PeriodSelectorWidget.h"
#include "database.h"

#include <QGridLayout>
#include <QAbstractItemView>
#include <QLabel>
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
}

void PeriodSelectorWidget::setupLayout()
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(6);

    auto* title = new QLabel("Период:", this);
    layout->addWidget(title, 0, 0);

    modeCombo = new QComboBox(this);
    modeCombo->addItem("Неделя цикла (1-4)");
    modeCombo->addItem("Календарная неделя");
    layout->addWidget(modeCombo, 0, 1, 1, 2);

    cycleWeekSpin = new QSpinBox(this);
    cycleWeekSpin->setMinimum(1);
    cycleWeekSpin->setMaximum(4);
    cycleWeekSpin->setFixedWidth(60);
    layout->addWidget(cycleWeekSpin, 1, 1);

    calendarWeekCombo = new QComboBox(this);
    calendarWeekCombo->setMinimumWidth(160);
    calendarWeekCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    calendarWeekCombo->setMinimumContentsLength(10);
    calendarWeekCombo->view()->setTextElideMode(Qt::ElideRight);
    calendarWeekCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(calendarWeekCombo, 1, 2);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);

    // По умолчанию показываем только cycleWeekSpin
    calendarWeekCombo->setVisible(false);
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

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ++index;
        const int id = sqlite3_column_int(stmt, 0);
        const char* startText = (const char*)sqlite3_column_text(stmt, 2);
        const char* endText = (const char*)sqlite3_column_text(stmt, 3);

        const QString start = startText ? QString::fromUtf8(startText) : QString();
        const QString end = endText ? QString::fromUtf8(endText) : QString();
        const QString display = QString("Календарная неделя %1 (%2 — %3)").arg(index).arg(start).arg(end);
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

void PeriodSelectorWidget::emitSelectionChanged()
{
    emit selectionChanged(m_currentSelection);
}
