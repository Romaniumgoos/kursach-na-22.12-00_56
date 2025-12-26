#pragma once

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
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
    void emitSelectionChanged();

private:
    Database* db = nullptr;

    QComboBox* modeCombo = nullptr;      // "Неделя цикла", "Календарная"
    QSpinBox* cycleWeekSpin = nullptr;   // 1-4
    QComboBox* calendarWeekCombo = nullptr;

    WeekSelection m_currentSelection;

    void setupLayout();
    void populateCalendarWeeks();
};
