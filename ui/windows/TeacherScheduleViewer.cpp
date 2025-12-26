#include "ui/windows/TeacherScheduleViewer.h"

#include "database.h"
#include "ui/widgets/PeriodSelectorWidget.h"
#include "ui/widgets/WeekGridScheduleWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>

TeacherScheduleViewer::TeacherScheduleViewer(Database* db, int teacherId, const QString& teacherName, QWidget* parent)
    : QWidget(parent), db(db), teacherId(teacherId), teacherName(teacherName)
{
    setupLayout();

    if (periodSelector) {
        currentSelection = periodSelector->currentSelection();
    }
    reload();
}

void TeacherScheduleViewer::setupLayout()
{
    setWindowTitle(QString("Расписание преподавателя: %1").arg(teacherName));
    setMinimumSize(980, 620);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    titleLabel = new QLabel(QString("%1").arg(teacherName), this);
    QFont f = titleLabel->font();
    f.setPointSize(14);
    f.setBold(true);
    titleLabel->setFont(f);
    mainLayout->addWidget(titleLabel);

    periodSelector = new PeriodSelectorWidget(db, this);
    mainLayout->addWidget(periodSelector);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);

    topRow->addWidget(new QLabel("Подгруппа:", this));
    subgroupCombo = new QComboBox(this);
    subgroupCombo->addItem("Все пары", 0);
    subgroupCombo->addItem("Подгруппа 1", 1);
    subgroupCombo->addItem("Подгруппа 2", 2);
    subgroupCombo->setMaximumWidth(160);
    topRow->addWidget(subgroupCombo);
    topRow->addStretch(1);

    mainLayout->addLayout(topRow);

    weekGrid = new WeekGridScheduleWidget(this);
    mainLayout->addWidget(weekGrid, 1);

    connect(periodSelector, &PeriodSelectorWidget::selectionChanged,
            this, &TeacherScheduleViewer::onPeriodChanged);

    connect(subgroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                currentSubgroup = subgroupCombo ? subgroupCombo->currentData().toInt() : 0;
                reload();
            });
}

void TeacherScheduleViewer::onPeriodChanged(const WeekSelection& selection)
{
    currentSelection = selection;
    reload();
}

void TeacherScheduleViewer::reload()
{
    if (!db || !weekGrid || teacherId <= 0) return;

    int resolvedWeekId = 0;
    int weekOfCycle = 1;

    if (currentSelection.mode == WeekSelection::CycleWeek) {
        weekOfCycle = currentSelection.weekOfCycle;
        resolvedWeekId = 0;
    } else if (currentSelection.mode == WeekSelection::CalendarWeek) {
        resolvedWeekId = currentSelection.weekId;
        if (resolvedWeekId <= 0) return;

        weekOfCycle = db->getWeekOfCycleByWeekId(resolvedWeekId);
        if (weekOfCycle <= 0) return;
    }

    weekGrid->setTeacherScheduleAllGroups(db, teacherId, weekOfCycle, resolvedWeekId, currentSubgroup);
}
