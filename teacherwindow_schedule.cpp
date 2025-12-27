#include "teacherwindow.h"

#include "ui/models/WeekSelection.h"
#include "ui/widgets/PeriodSelectorWidget.h"
#include "ui/widgets/WeekGridScheduleWidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

QWidget* TeacherWindow::buildScheduleTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);

    schedulePeriodSelector = new PeriodSelectorWidget(db, root);
    layout->addWidget(schedulePeriodSelector);

    auto* filtersRow = new QHBoxLayout();
    filtersRow->setContentsMargins(0, 0, 0, 0);

    filtersRow->addWidget(new QLabel("Группа:", root));
    scheduleGroupCombo = new QComboBox(root);
    scheduleGroupCombo->setMinimumWidth(120);
    scheduleGroupCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    scheduleGroupCombo->view()->setTextElideMode(Qt::ElideRight);
    scheduleGroupCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    filtersRow->addWidget(scheduleGroupCombo);

    filtersRow->addSpacing(12);
    filtersRow->addWidget(new QLabel("Подгруппа:", root));
    scheduleSubgroupCombo = new QComboBox(root);
    scheduleSubgroupCombo->addItem("Все", 0);
    scheduleSubgroupCombo->addItem("1", 1);
    scheduleSubgroupCombo->addItem("2", 2);
    scheduleSubgroupCombo->setMinimumWidth(80);
    scheduleSubgroupCombo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    filtersRow->addWidget(scheduleSubgroupCombo);

    filtersRow->addStretch();
    layout->addLayout(filtersRow);

    scheduleGrid = new WeekGridScheduleWidget(root);
    layout->addWidget(scheduleGrid);

    scheduleEmptyLabel = new QLabel("Выберите период и группу", root);
    scheduleEmptyLabel->setAlignment(Qt::AlignCenter);
    scheduleEmptyLabel->setStyleSheet("color: palette(mid); font-size: 14px;");
    layout->addWidget(scheduleEmptyLabel);

    connect(schedulePeriodSelector, &PeriodSelectorWidget::selectionChanged,
            this, &TeacherWindow::onSchedulePeriodChanged);
    connect(scheduleGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onScheduleGroupChanged);
    connect(scheduleSubgroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onScheduleSubgroupChanged);

    onSchedulePeriodChanged(schedulePeriodSelector->currentSelection());
    return root;
}

bool TeacherWindow::resolveWeekSelection(const WeekSelection& selection,
                                        int& outResolvedWeekId,
                                        int& outWeekOfCycle,
                                        QString& outErrorText)
{
    outResolvedWeekId = 0;
    outWeekOfCycle = 1;
    outErrorText.clear();

    if (!db) {
        outErrorText = "База данных не инициализирована.";
        return false;
    }

    if (selection.mode == WeekSelection::CycleWeek) {
        outWeekOfCycle = selection.weekOfCycle;
        outResolvedWeekId = 0;
        return true;
    }

    if (selection.mode == WeekSelection::CalendarWeek) {
        outResolvedWeekId = selection.weekId;
        if (outResolvedWeekId <= 0) {
            outErrorText = "Неделя не найдена (weekId=0).";
            return false;
        }
        outWeekOfCycle = db->getWeekOfCycleByWeekId(outResolvedWeekId);
        if (outWeekOfCycle <= 0) {
            outErrorText = "Неделя не найдена в cycleweeks.";
            return false;
        }
        return true;
    }

    outErrorText = "Некорректный режим выбора периода.";
    return false;
}

void TeacherWindow::onSchedulePeriodChanged(const WeekSelection&)
{
    reloadSchedule();
}

void TeacherWindow::onScheduleGroupChanged(int)
{
    reloadSchedule();
}

void TeacherWindow::onScheduleSubgroupChanged(int)
{
    reloadSchedule();
}

void TeacherWindow::reloadSchedule()
{
    if (!scheduleEmptyLabel || !schedulePeriodSelector || !scheduleGrid) return;

    const int groupId = scheduleGroupCombo ? scheduleGroupCombo->currentData().toInt() : 0;
    const bool allGroups = (groupId == 0);

    int weekId = 0;
    int weekOfCycle = 1;
    QString err;
    if (!resolveWeekSelection(schedulePeriodSelector->currentSelection(), weekId, weekOfCycle, err)) {
        scheduleEmptyLabel->setText(err);
        scheduleEmptyLabel->setVisible(true);
        scheduleGrid->setVisible(false);
        return;
    }

    const int subgroupFilter = scheduleSubgroupCombo ? scheduleSubgroupCombo->currentData().toInt() : 0;

    scheduleEmptyLabel->setVisible(false);
    scheduleGrid->setVisible(true);

    if (allGroups) {
        scheduleGrid->setTeacherScheduleAllGroups(db, teacherId, weekOfCycle, weekId, subgroupFilter);
    } else {
        const QString groupName = scheduleGroupCombo ? scheduleGroupCombo->currentText() : QString();
        scheduleGrid->setTeacherSchedule(db, teacherId, groupId, groupName, weekOfCycle, weekId, subgroupFilter);
    }
}
