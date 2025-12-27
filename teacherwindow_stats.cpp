#include "teacherwindow.h"

#include "ui/util/UiStyle.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QDate>

#include <tuple>
#include <vector>

void TeacherWindow::onStatsStudentSelected()
{
    if (!statsGradesTable) return;

    int studentId = 0;
    QString studentLabel;

    if (statsGradesTable->selectionModel() && !statsGradesTable->selectionModel()->selectedRows().isEmpty()) {
        const int row = statsGradesTable->selectionModel()->selectedRows().front().row();
        if (auto* it = statsGradesTable->item(row, 0)) {
            studentId = it->data(Qt::UserRole).toInt();
            studentLabel = it->text();
        }
    }

    if (studentId <= 0) return;
    if (statsDetailTitleLabel) {
        statsDetailTitleLabel->setText(QString("История: %1").arg(studentLabel));
    }
    reloadStatsStudentDetails(studentId);
}

void TeacherWindow::reloadStatsStudentDetails(int studentId)
{
    if (!db || studentId <= 0) return;
    if (!statsDetailGradesTable || !statsDetailAbsencesTable || !statsDetailGradesSummaryLabel || !statsDetailAbsencesSummaryLabel) return;
    if (!statsGroupCombo || !statsSubjectCombo) return;

    const int groupId = statsGroupCombo->currentData().toInt();
    const int semesterId = defaultSemesterId();
    const int subjectId = statsSubjectCombo->currentData().toInt();
    const QString subjectName = statsSubjectCombo->currentText();

    statsDetailGradesTable->setSortingEnabled(false);
    statsDetailAbsencesTable->setSortingEnabled(false);
    statsDetailGradesTable->setRowCount(0);
    statsDetailAbsencesTable->setRowCount(0);

    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    auto resolveLessonMeta = [&](const QString& dateISO, const QString& subj) {
        QString outTime = "—";
        QString outType = "—";
        QString outRoom = "—";

        if (!db || groupId <= 0 || dateISO.size() < 10 || subj.isEmpty()) {
            return std::make_tuple(outTime, outType, outRoom);
        }

        const QDate d = QDate::fromString(dateISO, "yyyy-MM-dd");
        if (!d.isValid()) {
            return std::make_tuple(outTime, outType, outRoom);
        }

        const int weekday = d.dayOfWeek();
        if (weekday < 1 || weekday > 6) {
            return std::make_tuple(outTime, outType, outRoom);
        }
        const int weekId = db->getWeekIdByDate(dateISO.toStdString());
        const int weekOfCycle = (weekId > 0) ? db->getWeekOfCycleByWeekId(weekId) : 0;
        if (weekOfCycle <= 0) {
            return std::make_tuple(outTime, outType, outRoom);
        }

        std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> sched;
        if (!db->getScheduleForGroup(groupId, weekday, weekOfCycle, sched)) {
            return std::make_tuple(outTime, outType, outRoom);
        }

        for (const auto& s : sched) {
            const int lessonNumber = std::get<1>(s);
            const QString sSubj = QString::fromStdString(std::get<3>(s));
            const QString sRoom = QString::fromStdString(std::get<4>(s));
            const QString sType = QString::fromStdString(std::get<5>(s));
            if (sSubj == subj) {
                if (lessonNumber >= 1 && lessonNumber <= pairTimes.size()) {
                    outTime = pairTimes[lessonNumber - 1];
                }
                if (!sType.isEmpty()) outType = sType;
                if (!sRoom.isEmpty()) outRoom = sRoom;
                break;
            }
        }

        return std::make_tuple(outTime, outType, outRoom);
    };

    // ===== detailed grades =====
    double sum = 0.0;
    int cnt = 0;
    if (subjectId > 0) {
        std::vector<std::tuple<int, std::string, std::string>> grades;
        if (db->getStudentSubjectGrades(studentId, subjectId, semesterId, grades)) {
            for (const auto& g : grades) {
                const int value = std::get<0>(g);
                const QString date = QString::fromStdString(std::get<1>(g));
                const QString gType = QString::fromStdString(std::get<2>(g));
                const auto [time, ltype, room] = resolveLessonMeta(date, subjectName);

                const int row = statsDetailGradesTable->rowCount();
                statsDetailGradesTable->insertRow(row);
                statsDetailGradesTable->setItem(row, 0, new QTableWidgetItem(date));
                statsDetailGradesTable->setItem(row, 1, new QTableWidgetItem(time));
                statsDetailGradesTable->setItem(row, 2, new QTableWidgetItem(subjectName));

                auto* badge = new QLabel(QString::number(value), statsDetailGradesTable);
                badge->setAlignment(Qt::AlignCenter);
                badge->setMinimumHeight(22);
                badge->setStyleSheet(UiStyle::badgeGradeStyle(value));
                statsDetailGradesTable->setCellWidget(row, 3, badge);

                if (!ltype.isEmpty() && ltype != "—") {
                    auto* ltBadge = new QLabel(ltype, statsDetailGradesTable);
                    ltBadge->setAlignment(Qt::AlignCenter);
                    ltBadge->setMinimumHeight(22);
                    ltBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(ltype));
                    statsDetailGradesTable->setCellWidget(row, 4, ltBadge);
                } else {
                    auto* it = new QTableWidgetItem("—");
                    it->setTextAlignment(Qt::AlignCenter);
                    statsDetailGradesTable->setItem(row, 4, it);
                }

                statsDetailGradesTable->setItem(row, 5, new QTableWidgetItem(room));
                statsDetailGradesTable->setItem(row, 6, new QTableWidgetItem(gType.isEmpty() ? "" : gType));

                sum += value;
                ++cnt;
            }
        }
    } else {
        std::vector<std::tuple<std::string, int, std::string, std::string>> grades;
        if (db->getStudentGradesForSemester(studentId, semesterId, grades)) {
            for (const auto& g : grades) {
                const QString subj = QString::fromStdString(std::get<0>(g));
                const int value = std::get<1>(g);
                const QString date = QString::fromStdString(std::get<2>(g));
                const QString gType = QString::fromStdString(std::get<3>(g));
                const auto [time, ltype, room] = resolveLessonMeta(date, subj);

                const int row = statsDetailGradesTable->rowCount();
                statsDetailGradesTable->insertRow(row);
                statsDetailGradesTable->setItem(row, 0, new QTableWidgetItem(date));
                statsDetailGradesTable->setItem(row, 1, new QTableWidgetItem(time));
                statsDetailGradesTable->setItem(row, 2, new QTableWidgetItem(subj));

                auto* badge = new QLabel(QString::number(value), statsDetailGradesTable);
                badge->setAlignment(Qt::AlignCenter);
                badge->setMinimumHeight(22);
                badge->setStyleSheet(UiStyle::badgeGradeStyle(value));
                statsDetailGradesTable->setCellWidget(row, 3, badge);

                if (!ltype.isEmpty() && ltype != "—") {
                    auto* ltBadge = new QLabel(ltype, statsDetailGradesTable);
                    ltBadge->setAlignment(Qt::AlignCenter);
                    ltBadge->setMinimumHeight(22);
                    ltBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(ltype));
                    statsDetailGradesTable->setCellWidget(row, 4, ltBadge);
                } else {
                    auto* it = new QTableWidgetItem("—");
                    it->setTextAlignment(Qt::AlignCenter);
                    statsDetailGradesTable->setItem(row, 4, it);
                }

                statsDetailGradesTable->setItem(row, 5, new QTableWidgetItem(room));
                statsDetailGradesTable->setItem(row, 6, new QTableWidgetItem(gType.isEmpty() ? "" : gType));

                sum += value;
                ++cnt;
            }
        }
    }

    const double avg = (cnt > 0) ? (sum / cnt) : 0.0;
    statsDetailGradesSummaryLabel->setText(QString("Оценки: %1 записей, средний %2")
                                          .arg(cnt)
                                          .arg(cnt > 0 ? QString::number(avg, 'f', 2) : "—"));

    // ===== detailed absences =====
    int totalH = 0;
    int unexcH = 0;
    std::vector<std::tuple<std::string, int, std::string, std::string>> abs;
    if (db->getStudentAbsencesForSemester(studentId, semesterId, abs)) {
        for (const auto& a : abs) {
            const QString subj = QString::fromStdString(std::get<0>(a));
            const int hours = std::get<1>(a);
            const QString date = QString::fromStdString(std::get<2>(a));
            const QString type = QString::fromStdString(std::get<3>(a));

            if (subjectId > 0 && subj != subjectName) continue;

            const auto [time, ltype, room] = resolveLessonMeta(date, subj);

            const int row = statsDetailAbsencesTable->rowCount();
            statsDetailAbsencesTable->insertRow(row);
            statsDetailAbsencesTable->setItem(row, 0, new QTableWidgetItem(date));
            statsDetailAbsencesTable->setItem(row, 1, new QTableWidgetItem(time));
            statsDetailAbsencesTable->setItem(row, 2, new QTableWidgetItem(subj));

            auto* hItem = new QTableWidgetItem(QString::number(hours));
            hItem->setTextAlignment(Qt::AlignCenter);
            statsDetailAbsencesTable->setItem(row, 3, hItem);

            const bool excused = (type == "excused");
            auto* typeBadge = new QLabel(excused ? "Уважительный" : "Неуважительный", statsDetailAbsencesTable);
            typeBadge->setAlignment(Qt::AlignCenter);
            typeBadge->setMinimumHeight(22);
            typeBadge->setStyleSheet(UiStyle::badgeAbsenceStyle(excused));
            statsDetailAbsencesTable->setCellWidget(row, 4, typeBadge);

            if (!ltype.isEmpty() && ltype != "—") {
                auto* ltBadge = new QLabel(ltype, statsDetailAbsencesTable);
                ltBadge->setAlignment(Qt::AlignCenter);
                ltBadge->setMinimumHeight(22);
                ltBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(ltype));
                statsDetailAbsencesTable->setCellWidget(row, 5, ltBadge);
            } else {
                auto* it = new QTableWidgetItem("—");
                it->setTextAlignment(Qt::AlignCenter);
                statsDetailAbsencesTable->setItem(row, 5, it);
            }

            statsDetailAbsencesTable->setItem(row, 6, new QTableWidgetItem(room));

            totalH += hours;
            if (!excused) unexcH += hours;
        }
    }

    statsDetailAbsencesSummaryLabel->setText(QString("Пропуски: всего %1 ч, неуважительных %2 ч")
                                            .arg(totalH)
                                            .arg(unexcH));

    statsDetailGradesTable->setSortingEnabled(true);
    statsDetailAbsencesTable->setSortingEnabled(true);
    statsDetailGradesTable->sortItems(0, Qt::DescendingOrder);
    statsDetailAbsencesTable->sortItems(0, Qt::DescendingOrder);
}

QWidget* TeacherWindow::buildGroupStatsTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* filtersCard = new QFrame(root);
    filtersCard->setFrameShape(QFrame::StyledPanel);
    filtersCard->setStyleSheet("QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Window);}"
                               "QLabel{color: palette(WindowText); background: transparent;}");
    auto* topRow = new QHBoxLayout(filtersCard);
    topRow->setContentsMargins(14, 10, 14, 10);
    topRow->setSpacing(10);

    auto* groupLabel = new QLabel("Группа", filtersCard);
    groupLabel->setStyleSheet(UiStyle::badgeNeutralStyle());
    groupLabel->setMinimumHeight(22);
    groupLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(groupLabel);
    statsGroupCombo = new QComboBox(filtersCard);
    statsGroupCombo->setMinimumWidth(180);
    statsGroupCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topRow->addWidget(statsGroupCombo);

    topRow->addSpacing(12);
    auto* subjectLabel = new QLabel("Предмет", filtersCard);
    subjectLabel->setStyleSheet(UiStyle::badgeNeutralStyle());
    subjectLabel->setMinimumHeight(22);
    subjectLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(subjectLabel);
    statsSubjectCombo = new QComboBox(filtersCard);
    statsSubjectCombo->setMinimumWidth(240);
    statsSubjectCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topRow->addWidget(statsSubjectCombo);

    topRow->addStretch();
    auto* refresh = new QPushButton("Обновить", filtersCard);
    topRow->addWidget(refresh);

    layout->addWidget(filtersCard);

    auto* splitter = new QSplitter(Qt::Horizontal, root);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(8);
    layout->addWidget(splitter, 1);

    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    auto* studentsBox = new QGroupBox("Студенты", leftPane);
    auto* studentsLayout = new QVBoxLayout(studentsBox);
    studentsLayout->setContentsMargins(10, 8, 10, 8);
    studentsLayout->setSpacing(8);

    statsGradesTable = new QTableWidget(studentsBox);
    UiStyle::applyStandardTableStyle(statsGradesTable);
    statsGradesTable->setColumnCount(1);
    statsGradesTable->setHorizontalHeaderLabels({"ФИО"});
    statsGradesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    statsGradesTable->setSortingEnabled(true);
    statsGradesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    statsGradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    studentsLayout->addWidget(statsGradesTable);

    statsGradesSummaryLabel = new QLabel(studentsBox);
    UiStyle::makeInfoLabel(statsGradesSummaryLabel);
    studentsLayout->addWidget(statsGradesSummaryLabel);

    // no second left table anymore
    statsAbsencesTable = nullptr;
    statsAbsencesSummaryLabel = nullptr;

    leftLayout->addWidget(studentsBox, 1);

    splitter->addWidget(leftPane);

    statsDetailWidget = new QWidget(splitter);
    auto* detailLayout = new QVBoxLayout(statsDetailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(12);

    auto* detailHeader = new QFrame(statsDetailWidget);
    detailHeader->setFrameShape(QFrame::StyledPanel);
    detailHeader->setStyleSheet("QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Window);}"
                                "QLabel{color: palette(WindowText); background: transparent;}");
    detailHeader->setMinimumHeight(52);
    auto* dh = new QHBoxLayout(detailHeader);
    dh->setContentsMargins(14, 10, 14, 10);
    dh->setSpacing(10);

    statsDetailTitleLabel = new QLabel("Выберите студента слева", detailHeader);
    statsDetailTitleLabel->setStyleSheet("font-weight: 900; font-size: 14px; color: palette(WindowText);");
    statsDetailTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statsDetailTitleLabel->setMinimumHeight(22);
    dh->addWidget(statsDetailTitleLabel, 1);

    auto* hint = new QLabel("детали", detailHeader);
    hint->setStyleSheet(UiStyle::badgeNeutralStyle());
    hint->setMinimumHeight(22);
    hint->setAlignment(Qt::AlignCenter);
    dh->addWidget(hint, 0, Qt::AlignRight);

    detailLayout->addWidget(detailHeader);

    auto* detailGradesBox = new QGroupBox("Все оценки студента", statsDetailWidget);
    detailGradesBox->setMinimumHeight(220);
    detailGradesBox->setStyleSheet(
        "QGroupBox{margin-top: 18px;}"
        "QGroupBox::title{subcontrol-origin: margin; subcontrol-position: top left;"
        "padding: 0 8px; left: 10px; color: palette(WindowText); font-weight: 800;}"
    );
    auto* dgLayout = new QVBoxLayout(detailGradesBox);
    dgLayout->setContentsMargins(10, 18, 10, 8);
    dgLayout->setSpacing(8);

    statsDetailGradesTable = new QTableWidget(detailGradesBox);
    UiStyle::applyStandardTableStyle(statsDetailGradesTable);
    statsDetailGradesTable->setColumnCount(7);
    statsDetailGradesTable->setHorizontalHeaderLabels({"Дата", "Время", "Предмет", "Оценка", "Занятие", "Ауд.", "Тип"});
    statsDetailGradesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsDetailGradesTable->horizontalHeader()->setStretchLastSection(true);
    statsDetailGradesTable->setSortingEnabled(true);
    statsDetailGradesTable->horizontalHeader()->setMinimumHeight(28);
    dgLayout->addWidget(statsDetailGradesTable);

    statsDetailGradesSummaryLabel = new QLabel(detailGradesBox);
    UiStyle::makeInfoLabel(statsDetailGradesSummaryLabel);
    dgLayout->addWidget(statsDetailGradesSummaryLabel);

    auto* detailAbsBox = new QGroupBox("Все пропуски студента", statsDetailWidget);
    detailAbsBox->setMinimumHeight(220);
    detailAbsBox->setStyleSheet(
        "QGroupBox{margin-top: 18px;}"
        "QGroupBox::title{subcontrol-origin: margin; subcontrol-position: top left;"
        "padding: 0 8px; left: 10px; color: palette(WindowText); font-weight: 800;}"
    );
    auto* daLayout = new QVBoxLayout(detailAbsBox);
    daLayout->setContentsMargins(10, 18, 10, 8);
    daLayout->setSpacing(8);

    statsDetailAbsencesTable = new QTableWidget(detailAbsBox);
    UiStyle::applyStandardTableStyle(statsDetailAbsencesTable);
    statsDetailAbsencesTable->setColumnCount(7);
    statsDetailAbsencesTable->setHorizontalHeaderLabels({"Дата", "Время", "Предмет", "Часы", "Тип", "Занятие", "Ауд."});
    statsDetailAbsencesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsDetailAbsencesTable->horizontalHeader()->setStretchLastSection(true);
    statsDetailAbsencesTable->setSortingEnabled(true);
    statsDetailAbsencesTable->horizontalHeader()->setMinimumHeight(28);
    statsDetailAbsencesTable->horizontalHeader()->setMinimumSectionSize(90);
    statsDetailAbsencesTable->setColumnWidth(4, 140);
    daLayout->addWidget(statsDetailAbsencesTable);

    statsDetailAbsencesSummaryLabel = new QLabel(detailAbsBox);
    UiStyle::makeInfoLabel(statsDetailAbsencesSummaryLabel);
    daLayout->addWidget(statsDetailAbsencesSummaryLabel);

    detailLayout->addWidget(detailGradesBox, 1);
    detailLayout->addWidget(detailAbsBox, 1);
    splitter->addWidget(statsDetailWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    statsSubjectCombo->addItem("Все предметы", 0);

    connect(statsGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onStatsGroupChanged);
    connect(statsSubjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onStatsSubjectChanged);
    connect(refresh, &QPushButton::clicked, this, &TeacherWindow::reloadGroupStats);

    connect(statsGradesTable, &QTableWidget::itemSelectionChanged,
            this, &TeacherWindow::onStatsStudentSelected);

    statsGradesSummaryLabel->setText("Студенты: —");
    statsDetailGradesSummaryLabel->setText("Оценки: —");
    statsDetailAbsencesSummaryLabel->setText("Пропуски: —");
    return root;
}

void TeacherWindow::reloadGroupStatsSubjects()
{
    if (!db || !statsSubjectCombo || !statsGroupCombo) return;

    const int groupId = statsGroupCombo->currentData().toInt();
    const QVariant current = statsSubjectCombo->currentData();

    statsSubjectCombo->blockSignals(true);
    statsSubjectCombo->clear();
    statsSubjectCombo->addItem("Все предметы", 0);

    if (groupId > 0) {
        std::vector<std::pair<int, std::string>> subjects;
        if (db->getSubjectsForTeacherInGroupSchedule(teacherId, groupId, subjects)) {
            for (const auto& s : subjects) {
                statsSubjectCombo->addItem(QString::fromStdString(s.second), s.first);
            }
        }
    }

    const int idx = statsSubjectCombo->findData(current);
    if (idx >= 0) statsSubjectCombo->setCurrentIndex(idx);
    statsSubjectCombo->blockSignals(false);
}

void TeacherWindow::reloadGroupStats()
{
    if (!db || !statsGroupCombo || !statsSubjectCombo) return;
    if (!statsGradesTable || !statsGradesSummaryLabel) return;

    if (statsDetailTitleLabel) statsDetailTitleLabel->setText("Выберите студента слева");
    if (statsDetailGradesTable) statsDetailGradesTable->setRowCount(0);
    if (statsDetailAbsencesTable) statsDetailAbsencesTable->setRowCount(0);
    if (statsDetailGradesSummaryLabel) statsDetailGradesSummaryLabel->setText("Оценки: —");
    if (statsDetailAbsencesSummaryLabel) statsDetailAbsencesSummaryLabel->setText("Пропуски: —");

    const int groupId = statsGroupCombo->currentData().toInt();
    const int semesterId = defaultSemesterId();

    statsGradesTable->setRowCount(0);

    if (groupId <= 0 || semesterId <= 0) {
        statsGradesSummaryLabel->setText("Студенты: выберите группу и семестр");
        return;
    }

    std::vector<std::pair<int, std::string>> students;
    if (!db->getStudentsOfGroup(groupId, students)) {
        statsGradesSummaryLabel->setText("Студенты: не удалось загрузить");
        return;
    }

    for (const auto& st : students) {
        const int studentId = st.first;
        const QString studentName = QString::fromStdString(st.second);

        const int row = statsGradesTable->rowCount();
        statsGradesTable->insertRow(row);
        auto* nameItem = new QTableWidgetItem(studentName.isEmpty() ? QString("ID %1").arg(studentId) : studentName);
        nameItem->setData(Qt::UserRole, studentId);
        statsGradesTable->setItem(row, 0, nameItem);
    }

    statsGradesSummaryLabel->setText(QString("Студенты: %1").arg(statsGradesTable->rowCount()));
    statsGradesTable->sortItems(0, Qt::AscendingOrder);
}

void TeacherWindow::onStatsGroupChanged(int)
{
    reloadGroupStatsSubjects();
    reloadGroupStats();
}

void TeacherWindow::onStatsSubjectChanged(int)
{
    reloadGroupStats();
}
