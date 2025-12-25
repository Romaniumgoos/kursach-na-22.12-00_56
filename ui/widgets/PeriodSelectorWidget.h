#pragma once

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QPushButton>
#include "ui/models/WeekSelection.h"

class Database;

class PeriodSelectorWidget : public QWidget {
    Q_OBJECT
public:
    explicit PeriodSelectorWidget(Database* db, QWidget* parent = nullptr);

    WeekSelection currentSelection() const;

    signals:
        void selectionChanged(const WeekSelection& selection);

private slots:
    void onModeChanged(int index);
    void onCycleWeekChanged(int value);
    void onCalendarWeekChanged(int index);
    void onDateChanged(const QDate& date);
    void emitSelectionChanged();

private:
    Database* db = nullptr;

    QComboBox* modeCombo = nullptr;      // "Неделя цикла", "Календарная", "По дате"
    QSpinBox* cycleWeekSpin = nullptr;   // 1-4
    QComboBox* calendarWeekCombo = nullptr;
    QDateEdit* dateEdit = nullptr;

    WeekSelection m_currentSelection;

    void setupLayout();
    void populateCalendarWeeks();
};
