#include "adminwindow.h"

#include "ui/util/UiStyle.h"
#include "ui/util/AppEvents.h"
#include "ui/models/WeekSelection.h"
#include "ui/widgets/PeriodSelectorWidget.h"
#include "ui/widgets/WeekGridScheduleWidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

#include <sqlite3.h>

namespace {

static QString cardFrameStyle()
{
    return "QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}"
           "QLabel{color: palette(Text); background: transparent;}";
}

static QLabel* makeBadge(QWidget* parent, const QString& text, const QString& style)
{
    auto* b = new QLabel(text, parent);
    b->setAlignment(Qt::AlignCenter);
    b->setMinimumHeight(22);
    b->setStyleSheet(style);
    return b;
}

static QString weekdayName(int weekday)
{
    switch (weekday) {
    case 1: return "Пн";
    case 2: return "Вт";
    case 3: return "Ср";
    case 4: return "Чт";
    case 5: return "Пт";
    case 6: return "Сб";
    default: return QString::number(weekday);
    }
}

static bool resolveWeekSelectionForAdmin(Database* db,
                                        const WeekSelection& selection,
                                        int& outResolvedWeekId,
                                        int& outWeekOfCycle,
                                        QString& outErrorText)
{
    outResolvedWeekId = 0;
    outWeekOfCycle = 1;
    outErrorText.clear();

    if (!db) {
        outErrorText = "Нет соединения с БД.";
        return false;
    }

    if (selection.mode == WeekSelection::CycleWeek) {
        outWeekOfCycle = selection.weekOfCycle;
        return true;
    }

    if (selection.mode == WeekSelection::CalendarWeek) {
        outResolvedWeekId = selection.weekId;
        if (outResolvedWeekId <= 0) {
            outErrorText = "Не выбрана календарная неделя.";
            return false;
        }
        outWeekOfCycle = db->getWeekOfCycleByWeekId(outResolvedWeekId);
        if (outWeekOfCycle <= 0) {
            outErrorText = "Не удалось определить неделю цикла по выбранной календарной неделе.";
            return false;
        }
        return true;
    }

    outErrorText = "Неизвестный режим выбора периода.";
    return false;
}

struct ScheduleEditResult {
    bool accepted = false;
    int id = 0;
    int groupId = 0;
    int subgroup = 0;
    int weekday = 1;
    int lessonNumber = 1;
    int weekOfCycle = 1;
    int subjectId = 0;
    int teacherId = 0;
    QString room;
    QString lessonType;
};

static bool execGetScheduleById(sqlite3* rawDb, int scheduleId, ScheduleEditResult& out)
{
    if (!rawDb || scheduleId <= 0) return false;
    const char* sql = R"SQL(
        SELECT id, groupid, subgroup, weekday, lessonnumber, weekofcycle,
               subjectid, teacherid, COALESCE(room,''), COALESCE(lessontype,'')
        FROM schedule
        WHERE id = ?
    )SQL";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, scheduleId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out.id = sqlite3_column_int(stmt, 0);
        out.groupId = sqlite3_column_int(stmt, 1);
        out.subgroup = sqlite3_column_int(stmt, 2);
        out.weekday = sqlite3_column_int(stmt, 3);
        out.lessonNumber = sqlite3_column_int(stmt, 4);
        out.weekOfCycle = sqlite3_column_int(stmt, 5);
        out.subjectId = sqlite3_column_int(stmt, 6);
        out.teacherId = sqlite3_column_int(stmt, 7);
        out.room = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        out.lessonType = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

static bool execIsTeacherBusyExcluding(sqlite3* rawDb, int teacherId, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = "SELECT COUNT(*) FROM schedule WHERE teacherid=? AND weekday=? AND lessonnumber=? AND weekofcycle=? AND id<>?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static bool execIsRoomBusyExcluding(sqlite3* rawDb, const QString& room, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = "SELECT COUNT(*) FROM schedule WHERE room=? AND weekday=? AND lessonnumber=? AND weekofcycle=? AND room IS NOT NULL AND room<>'' AND id<>?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, room.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static bool execIsSlotBusyExcluding(sqlite3* rawDb, int groupId, int subgroup, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = R"SQL(
        SELECT COUNT(*)
        FROM schedule
        WHERE groupid = ?
          AND weekday = ?
          AND lessonnumber = ?
          AND weekofcycle = ?
          AND (subgroup = ? OR subgroup = 0)
          AND id <> ?
    )SQL";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, subgroup);
    sqlite3_bind_int(stmt, 6, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static ScheduleEditResult runScheduleEditDialog(QWidget* parent, Database* db, const QString& title, bool isCreate, const ScheduleEditResult& initial)
{
    ScheduleEditResult res = initial;

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setModal(true);

    auto* root = new QVBoxLayout(&dlg);
    auto* card = new QFrame(&dlg);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(cardFrameStyle());
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(10);
    root->addWidget(card);

    auto* form = new QFormLayout();
    form->setSpacing(10);
    cardLayout->addLayout(form);

    auto* groupCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                groupCombo->addItem(QString::fromStdString(g.second), g.first);
            }
        }
    }
    const int gi = groupCombo->findData(initial.groupId);
    if (gi >= 0) groupCombo->setCurrentIndex(gi);
    form->addRow("Группа:", groupCombo);

    auto* weekdayCombo = new QComboBox(&dlg);
    for (int d = 1; d <= 6; ++d) {
        weekdayCombo->addItem(weekdayName(d), d);
    }
    const int wi = weekdayCombo->findData(initial.weekday);
    if (wi >= 0) weekdayCombo->setCurrentIndex(wi);
    form->addRow("День:", weekdayCombo);

    auto* lessonSpin = new QSpinBox(&dlg);
    lessonSpin->setRange(1, 8);
    lessonSpin->setValue(initial.lessonNumber);
    form->addRow("Пара №:", lessonSpin);

    auto* weekCombo = new QComboBox(&dlg);
    weekCombo->addItem("1", 1);
    weekCombo->addItem("2", 2);
    weekCombo->addItem("3", 3);
    weekCombo->addItem("4", 4);
    const int wci = weekCombo->findData(initial.weekOfCycle);
    if (wci >= 0) weekCombo->setCurrentIndex(wci);
    form->addRow("Неделя цикла:", weekCombo);

    auto* subjectCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> subjects;
        if (db->getAllSubjects(subjects)) {
            for (const auto& s : subjects) {
                subjectCombo->addItem(QString::fromStdString(s.second), s.first);
            }
        }
    }
    const int si = subjectCombo->findData(initial.subjectId);
    if (si >= 0) subjectCombo->setCurrentIndex(si);
    form->addRow("Предмет:", subjectCombo);

    auto* teacherCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> teachers;
        if (db->getAllTeachers(teachers)) {
            for (const auto& t : teachers) {
                teacherCombo->addItem(QString::fromStdString(t.second), t.first);
            }
        }
    }
    const int ti = teacherCombo->findData(initial.teacherId);
    if (ti >= 0) teacherCombo->setCurrentIndex(ti);
    form->addRow("Преподаватель:", teacherCombo);

    auto* roomEdit = new QLineEdit(&dlg);
    roomEdit->setText(initial.room);
    form->addRow("Аудитория:", roomEdit);

    auto* typeCombo = new QComboBox(&dlg);
    typeCombo->addItem("ЛК", "ЛК");
    typeCombo->addItem("ПЗ", "ПЗ");
    typeCombo->addItem("ЛР", "ЛР");
    const int lti = typeCombo->findData(initial.lessonType);
    if (lti >= 0) typeCombo->setCurrentIndex(lti);
    form->addRow("Тип:", typeCombo);

    auto* subgroupCombo = new QComboBox(&dlg);
    subgroupCombo->addItem("0", 0);
    subgroupCombo->addItem("1", 1);
    subgroupCombo->addItem("2", 2);
    const int sgi = subgroupCombo->findData(initial.subgroup);
    if (sgi >= 0) subgroupCombo->setCurrentIndex(sgi);
    form->addRow("Подгруппа:", subgroupCombo);

    auto updateLk = [&]() {
        const bool isLk = typeCombo->currentData().toString() == "ЛК";
        subgroupCombo->setEnabled(!isLk);
        if (isLk) {
            const int idx = subgroupCombo->findData(0);
            if (idx >= 0) subgroupCombo->setCurrentIndex(idx);
        }
    };
    updateLk();
    QObject::connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&](int) {
        updateLk();
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        res.accepted = false;
        return res;
    }

    res.groupId = groupCombo->currentData().toInt();
    res.weekday = weekdayCombo->currentData().toInt();
    res.lessonNumber = lessonSpin->value();
    res.weekOfCycle = weekCombo->currentData().toInt();
    res.subjectId = subjectCombo->currentData().toInt();
    res.teacherId = teacherCombo->currentData().toInt();
    res.room = roomEdit->text().trimmed();
    res.lessonType = typeCombo->currentData().toString();
    res.subgroup = subgroupCombo->currentData().toInt();
    if (res.lessonType == "ЛК") {
        res.subgroup = 0;
    }
    res.accepted = true;
    return res;
}

} // namespace

QWidget* AdminWindow::buildScheduleTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* mainCard = new QFrame(root);
    mainCard->setFrameShape(QFrame::StyledPanel);
    mainCard->setStyleSheet(cardFrameStyle());
    auto* mainCardLayout = new QVBoxLayout(mainCard);
    mainCardLayout->setContentsMargins(12, 12, 12, 12);
    mainCardLayout->setSpacing(12);
    layout->addWidget(mainCard, 1);

    auto* controlsCard = new QFrame(mainCard);
    controlsCard->setFrameShape(QFrame::StyledPanel);
    controlsCard->setStyleSheet(cardFrameStyle());
    auto* controls = new QGridLayout(controlsCard);
    controls->setContentsMargins(14, 10, 14, 10);
    controls->setHorizontalSpacing(10);
    controls->setVerticalSpacing(8);

    controls->addWidget(makeBadge(controlsCard, "Режим", UiStyle::badgeNeutralStyle()), 0, 0);
    schedModeCombo = new QComboBox(controlsCard);
    schedModeCombo->setMinimumWidth(170);
    schedModeCombo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    schedModeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    schedModeCombo->addItem("Группа", 0);
    schedModeCombo->addItem("Преподаватель", 1);
    controls->addWidget(schedModeCombo, 0, 1);

    controls->addWidget(makeBadge(controlsCard, "Подгруппа", UiStyle::badgeNeutralStyle()), 0, 2);
    schedSubgroupCombo = new QComboBox(controlsCard);
    schedSubgroupCombo->setMinimumWidth(80);
    schedSubgroupCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    schedSubgroupCombo->addItem("Все", 0);
    schedSubgroupCombo->addItem("1", 1);
    schedSubgroupCombo->addItem("2", 2);
    controls->addWidget(schedSubgroupCombo, 0, 3);

    schedPeriodSelector = new PeriodSelectorWidget(db, controlsCard);
    schedPeriodSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controls->addWidget(schedPeriodSelector, 0, 4, 1, 3);

    controls->addWidget(makeBadge(controlsCard, "Группа", UiStyle::badgeNeutralStyle()), 1, 0);
    schedGroupCombo = new QComboBox(controlsCard);
    schedGroupCombo->setMinimumWidth(120);
    schedGroupCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    schedGroupCombo->view()->setTextElideMode(Qt::ElideRight);
    schedGroupCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    schedGroupCombo->setMinimumContentsLength(10);
    controls->addWidget(schedGroupCombo, 1, 1, 1, 2);

    controls->addWidget(makeBadge(controlsCard, "Преподаватель", UiStyle::badgeNeutralStyle()), 1, 3);
    schedTeacherCombo = new QComboBox(controlsCard);
    schedTeacherCombo->setMinimumWidth(140);
    schedTeacherCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    schedTeacherCombo->view()->setTextElideMode(Qt::ElideRight);
    schedTeacherCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    schedTeacherCombo->setMinimumContentsLength(12);
    controls->addWidget(schedTeacherCombo, 1, 4, 1, 2);

    addScheduleButton = new QPushButton("Добавить", controlsCard);
    editScheduleButton = new QPushButton("Редактировать", controlsCard);
    deleteScheduleButton = new QPushButton("Удалить", controlsCard);
    refreshScheduleButton = new QPushButton("Обновить", controlsCard);
    editScheduleButton->setEnabled(false);
    deleteScheduleButton->setEnabled(false);

    controls->addWidget(addScheduleButton, 1, 6);
    controls->addWidget(editScheduleButton, 1, 7);
    controls->addWidget(deleteScheduleButton, 1, 8);
    controls->addWidget(refreshScheduleButton, 1, 9);

    controls->setColumnStretch(0, 0);
    controls->setColumnStretch(1, 0);
    controls->setColumnStretch(2, 0);
    controls->setColumnStretch(3, 0);
    controls->setColumnStretch(4, 1);
    controls->setColumnStretch(5, 1);
    controls->setColumnStretch(6, 0);
    controls->setColumnStretch(7, 0);
    controls->setColumnStretch(8, 0);
    controls->setColumnStretch(9, 0);

    mainCardLayout->addWidget(controlsCard);

    auto* tableCard = new QFrame(mainCard);
    tableCard->setFrameShape(QFrame::StyledPanel);
    tableCard->setStyleSheet(cardFrameStyle());
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(12, 10, 12, 12);
    tableLayout->setSpacing(10);

    scheduleGrid = new WeekGridScheduleWidget(tableCard);
    tableLayout->addWidget(scheduleGrid, 1);

    mainCardLayout->addWidget(tableCard, 1);

    reloadGroupsInto(schedGroupCombo, false, true);
    reloadTeachersInto(schedTeacherCombo, false);

    auto updateModeUi = [this]() {
        const int mode = schedModeCombo ? schedModeCombo->currentData().toInt() : 0;
        const bool teacherMode = (mode == 1);
        if (schedGroupCombo) schedGroupCombo->setEnabled(!teacherMode);
        if (schedTeacherCombo) schedTeacherCombo->setEnabled(teacherMode);
    };
    updateModeUi();

    connect(refreshScheduleButton, &QPushButton::clicked, this, &AdminWindow::reloadSchedule);
    connect(addScheduleButton, &QPushButton::clicked, this, &AdminWindow::onAddSchedule);
    connect(editScheduleButton, &QPushButton::clicked, this, &AdminWindow::onEditSchedule);
    connect(deleteScheduleButton, &QPushButton::clicked, this, &AdminWindow::onDeleteSchedule);

    connect(schedModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updateModeUi](int) {
        updateModeUi();
        reloadSchedule();
    });
    connect(schedGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);
    connect(schedTeacherCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);
    connect(schedSubgroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);
    if (schedPeriodSelector) {
        connect(schedPeriodSelector, &PeriodSelectorWidget::selectionChanged, this, &AdminWindow::reloadSchedule);
    }

    if (scheduleGrid) {
        connect(scheduleGrid, &WeekGridScheduleWidget::lessonClicked, this, [this](int scheduleId) {
            selectedScheduleId = scheduleId;
            if (editScheduleButton) editScheduleButton->setEnabled(selectedScheduleId > 0);
            if (deleteScheduleButton) deleteScheduleButton->setEnabled(selectedScheduleId > 0);
        });
    }

    reloadSchedule();
    return root;
}

void AdminWindow::reloadSchedule()
{
    if (!db || !scheduleGrid || !schedModeCombo || !schedSubgroupCombo || !schedPeriodSelector) return;

    selectedScheduleId = 0;
    if (editScheduleButton) editScheduleButton->setEnabled(false);
    if (deleteScheduleButton) deleteScheduleButton->setEnabled(false);

    const int mode = schedModeCombo->currentData().toInt();
    const int subgroupFilter = schedSubgroupCombo->currentData().toInt();

    int resolvedWeekId = 0;
    int weekOfCycle = 1;
    QString err;
    if (!resolveWeekSelectionForAdmin(db, schedPeriodSelector->currentSelection(), resolvedWeekId, weekOfCycle, err)) {
        scheduleGrid->setVisible(false);
        return;
    }
    if (weekOfCycle <= 0) return;

    if (mode == 1) {
        if (!schedTeacherCombo) return;
        const int teacherId = schedTeacherCombo->currentData().toInt();
        if (teacherId <= 0) return;
        scheduleGrid->setTeacherScheduleAllGroups(db, teacherId, weekOfCycle, resolvedWeekId, subgroupFilter);
        return;
    }

    if (!schedGroupCombo) return;
    const int groupId = schedGroupCombo->currentData().toInt();
    if (groupId <= 0) return;
    scheduleGrid->setSchedule(db, groupId, weekOfCycle, resolvedWeekId, subgroupFilter);
}

void AdminWindow::reloadGroupsInto(QComboBox* combo, bool withAllOption, bool hideGroupIdZero)
{
    if (!combo) return;
    combo->blockSignals(true);
    const QVariant prev = combo->currentData();
    combo->clear();
    combo->addItem(withAllOption ? "Все" : "Выберите группу", withAllOption ? 0 : -1);

    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                if (hideGroupIdZero && g.first == 0) continue;
                combo->addItem(QString::fromStdString(g.second), g.first);
            }
        }
    }

    const int idx = combo->findData(prev);
    if (idx >= 0) combo->setCurrentIndex(idx);
    combo->blockSignals(false);
}

void AdminWindow::reloadTeachersInto(QComboBox* combo, bool withAllOption)
{
    if (!combo) return;
    combo->blockSignals(true);
    const QVariant prev = combo->currentData();
    combo->clear();
    combo->addItem(withAllOption ? "Все" : "Выберите преподавателя", withAllOption ? 0 : -1);

    if (db) {
        std::vector<std::pair<int, std::string>> teachers;
        if (db->getAllTeachers(teachers)) {
            for (const auto& t : teachers) {
                combo->addItem(QString::fromStdString(t.second), t.first);
            }
        }
    }

    const int idx = combo->findData(prev);
    if (idx >= 0) combo->setCurrentIndex(idx);
    combo->blockSignals(false);
}

void AdminWindow::onAddSchedule()
{
    if (!db) return;

    ScheduleEditResult init;
    if (schedModeCombo && schedModeCombo->currentData().toInt() == 0) {
        if (schedGroupCombo) init.groupId = schedGroupCombo->currentData().toInt();
    }
    if (schedSubgroupCombo) init.subgroup = schedSubgroupCombo->currentData().toInt();
    if (schedPeriodSelector) {
        int resolvedWeekId = 0;
        QString err;
        resolveWeekSelectionForAdmin(db, schedPeriodSelector->currentSelection(), resolvedWeekId, init.weekOfCycle, err);
    }

    ScheduleEditResult res = runScheduleEditDialog(this, db, "Добавить запись", true, init);
    if (!res.accepted) return;

    if (res.groupId < 0 || res.subjectId <= 0 || res.teacherId <= 0) {
        QMessageBox::warning(this, "Расписание", "Заполните обязательные поля.");
        return;
    }
    if (res.weekday < 1 || res.weekday > 6 || res.lessonNumber < 1 || res.lessonNumber > 8 || res.weekOfCycle < 1 || res.weekOfCycle > 4) {
        QMessageBox::warning(this, "Расписание", "Некорректные значения дня/пары/недели.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Расписание", "Подгруппа должна быть 0/1/2.");
        return;
    }

    if (db->isTeacherBusy(res.teacherId, res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: преподаватель занят в этот слот.");
        return;
    }
    if (!res.room.isEmpty() && db->isRoomBusy(res.room.toStdString(), res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: аудитория занята в этот слот.");
        return;
    }
    if (db->isScheduleSlotBusy(res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: у группы уже есть занятие в этот слот.");
        return;
    }

    if (!db->addScheduleEntry(res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle,
                             res.subjectId, res.teacherId, res.room.toStdString(), res.lessonType.toStdString())) {
        QMessageBox::critical(this, "Расписание", "Не удалось добавить запись.");
        return;
    }
    reloadSchedule();
    AppEvents::instance().emitScheduleChanged();
}

void AdminWindow::onEditSchedule()
{
    if (!db) return;
    const int id = selectedScheduleId;
    if (id <= 0) return;

    ScheduleEditResult init;
    if (!execGetScheduleById(db->getHandle(), id, init)) {
        QMessageBox::critical(this, "Расписание", "Не удалось загрузить запись для редактирования.");
        return;
    }

    ScheduleEditResult res = runScheduleEditDialog(this, db, "Редактировать запись", false, init);
    if (!res.accepted) return;

    if (res.groupId < 0 || res.subjectId <= 0 || res.teacherId <= 0) {
        QMessageBox::warning(this, "Расписание", "Заполните обязательные поля.");
        return;
    }
    if (res.weekday < 1 || res.weekday > 6 || res.lessonNumber < 1 || res.lessonNumber > 8 || res.weekOfCycle < 1 || res.weekOfCycle > 4) {
        QMessageBox::warning(this, "Расписание", "Некорректные значения дня/пары/недели.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Расписание", "Подгруппа должна быть 0/1/2.");
        return;
    }

    bool busy = false;
    if (!execIsTeacherBusyExcluding(db->getHandle(), res.teacherId, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
        QMessageBox::critical(this, "Расписание", "Ошибка проверки преподавателя.");
        return;
    }
    if (busy) {
        QMessageBox::warning(this, "Расписание", "Конфликт: преподаватель занят в этот слот.");
        return;
    }

    if (!res.room.isEmpty()) {
        if (!execIsRoomBusyExcluding(db->getHandle(), res.room, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
            QMessageBox::critical(this, "Расписание", "Ошибка проверки аудитории.");
            return;
        }
        if (busy) {
            QMessageBox::warning(this, "Расписание", "Конфликт: аудитория занята в этот слот.");
            return;
        }
    }

    if (!execIsSlotBusyExcluding(db->getHandle(), res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
        QMessageBox::critical(this, "Расписание", "Ошибка проверки слота группы.");
        return;
    }
    if (busy) {
        QMessageBox::warning(this, "Расписание", "Конфликт: у группы уже есть занятие в этот слот.");
        return;
    }

    if (!db->updateScheduleEntry(id, res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle,
                                res.subjectId, res.teacherId, res.room.toStdString(), res.lessonType.toStdString())) {
        QMessageBox::critical(this, "Расписание", "Не удалось сохранить изменения.");
        return;
    }

    reloadSchedule();
    AppEvents::instance().emitScheduleChanged();
}

void AdminWindow::onDeleteSchedule()
{
    if (!db) return;
    const int id = selectedScheduleId;
    if (id <= 0) return;

    if (QMessageBox::question(this, "Расписание", "Удалить запись?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!db->deleteScheduleEntry(id)) {
        QMessageBox::critical(this, "Расписание", "Не удалось удалить запись.");
        return;
    }
    reloadSchedule();
    AppEvents::instance().emitScheduleChanged();
}
