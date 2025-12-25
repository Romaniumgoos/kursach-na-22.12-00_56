#include "teacherwindow.h"
 #include "ui/widgets/PeriodSelectorWidget.h"
 #include "ui/widgets/ThemeToggleWidget.h"
 #include "ui/widgets/WeekGridScheduleWidget.h"
 #include "ui/util/UiStyle.h"
 #include "loginwindow.h"

 #include <QVBoxLayout>
 #include <QHBoxLayout>
 #include <QToolBar>
 #include <QAction>
 #include <QTabWidget>
 #include <QComboBox>
 #include <QTableWidget>
 #include <QHeaderView>
 #include <QFont>
 #include <QItemSelectionModel>
 #include <QSpinBox>
 #include <QPushButton>
 #include <QMessageBox>
 #include <QStackedWidget>
 #include <QDateEdit>
 #include <QDate>
 #include <QFrame>
 #include <QGridLayout>
 #include <QGroupBox>
 #include <QDialog>
 #include <QDialogButtonBox>
 #include <QFormLayout>
 #include <QSplitter>
 #include <QScrollArea>

 static QString lessonTypeBadgeStyle(const QString& lessonType)
 {
     return UiStyle::badgeLessonTypeStyle(lessonType);
 }

void TeacherWindow::onStatsStudentSelected()
{
    if (!statsGradesTable || !statsAbsencesTable) return;

    int studentId = 0;
    QString studentLabel;

    if (statsGradesTable->selectionModel() && !statsGradesTable->selectionModel()->selectedRows().isEmpty()) {
        const int row = statsGradesTable->selectionModel()->selectedRows().front().row();
        if (auto* it = statsGradesTable->item(row, 0)) {
            studentId = it->data(Qt::UserRole).toInt();
            studentLabel = it->text();
        }
    } else if (statsAbsencesTable->selectionModel() && !statsAbsencesTable->selectionModel()->selectedRows().isEmpty()) {
        const int row = statsAbsencesTable->selectionModel()->selectedRows().front().row();
        if (auto* it = statsAbsencesTable->item(row, 0)) {
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
    if (!statsGroupCombo || !statsSemesterCombo || !statsSubjectCombo) return;

    const int groupId = statsGroupCombo->currentData().toInt();
    const int semesterId = statsSemesterCombo->currentData().toInt();
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

        const int weekday = d.dayOfWeek() - 1;
        const int weekId = db->getWeekIdByDate(dateISO.toStdString());
        const int weekOfCycle = (weekId > 0) ? db->getWeekOfCycleByWeekId(weekId) : 0;
        if (weekday < 0 || weekOfCycle <= 0) {
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

 static QString neutralBadgeStyle()
 {
     return UiStyle::badgeNeutralStyle();
 }

TeacherWindow::TeacherWindow(Database* db, int teacherId, const QString& teacherName,
                             QWidget *parent)
    : QMainWindow(parent), db(db), teacherId(teacherId), teacherName(teacherName) {

    setupUI();

    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    auto* themeToggle = new ThemeToggleWidget(this);
    tb->addWidget(themeToggle);

    auto* logoutAction = new QAction("Выйти в авторизацию", this);
    tb->addAction(logoutAction);
    connect(logoutAction, &QAction::triggered, this, [this]() {
        this->hide();
        auto* login = new LoginWindow(this->db);
        login->setAttribute(Qt::WA_DeleteOnClose);
        login->show();
        this->close();
    });

    setWindowTitle(QString("Преподаватель: %1").arg(teacherName));
    resize(1000, 700);
}

TeacherWindow::~TeacherWindow() {
}

void TeacherWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(teacherName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    tabWidget->addTab(buildScheduleTab(), "📅 Расписание");
    tabWidget->addTab(buildJournalTab(), "📒 Журнал");
    tabWidget->addTab(buildGroupStatsTab(), "📊 Статистика");

    loadTeacherGroups();
}

QWidget* TeacherWindow::buildGroupStatsTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* filtersCard = new QFrame(root);
    filtersCard->setFrameShape(QFrame::StyledPanel);
    filtersCard->setStyleSheet("QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}"
                               "QLabel{color: palette(Text); background: transparent;}");
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
    topRow->addWidget(statsGroupCombo);

    topRow->addSpacing(12);
    auto* semesterLabel = new QLabel("Семестр", filtersCard);
    semesterLabel->setStyleSheet(UiStyle::badgeNeutralStyle());
    semesterLabel->setMinimumHeight(22);
    semesterLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(semesterLabel);
    statsSemesterCombo = new QComboBox(filtersCard);
    statsSemesterCombo->setMinimumWidth(160);
    topRow->addWidget(statsSemesterCombo);

    topRow->addSpacing(12);
    auto* subjectLabel = new QLabel("Предмет", filtersCard);
    subjectLabel->setStyleSheet(UiStyle::badgeNeutralStyle());
    subjectLabel->setMinimumHeight(22);
    subjectLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(subjectLabel);
    statsSubjectCombo = new QComboBox(filtersCard);
    statsSubjectCombo->setMinimumWidth(240);
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

    auto* gradesBox = new QGroupBox("Оценки", leftPane);
    auto* gradesLayout = new QVBoxLayout(gradesBox);
    gradesLayout->setContentsMargins(10, 8, 10, 8);
    gradesLayout->setSpacing(8);

    statsGradesTable = new QTableWidget(gradesBox);
    UiStyle::applyStandardTableStyle(statsGradesTable);
    statsGradesTable->setColumnCount(3);
    statsGradesTable->setHorizontalHeaderLabels({"Студент", "Средний", "Кол-во"});
    statsGradesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsGradesTable->horizontalHeader()->setStretchLastSection(true);
    statsGradesTable->setSortingEnabled(true);
    statsGradesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    statsGradesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    gradesLayout->addWidget(statsGradesTable);

    statsGradesSummaryLabel = new QLabel(gradesBox);
    UiStyle::makeInfoLabel(statsGradesSummaryLabel);
    gradesLayout->addWidget(statsGradesSummaryLabel);

    auto* absBox = new QGroupBox("Пропуски", leftPane);
    auto* absLayout = new QVBoxLayout(absBox);
    absLayout->setContentsMargins(10, 8, 10, 8);
    absLayout->setSpacing(8);

    statsAbsencesTable = new QTableWidget(absBox);
    UiStyle::applyStandardTableStyle(statsAbsencesTable);
    statsAbsencesTable->setColumnCount(3);
    statsAbsencesTable->setHorizontalHeaderLabels({"Студент", "Всего (ч)", "Неуваж. (ч)"});
    statsAbsencesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsAbsencesTable->horizontalHeader()->setStretchLastSection(true);
    statsAbsencesTable->setSortingEnabled(true);
    statsAbsencesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    statsAbsencesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    absLayout->addWidget(statsAbsencesTable);

    statsAbsencesSummaryLabel = new QLabel(absBox);
    UiStyle::makeInfoLabel(statsAbsencesSummaryLabel);
    absLayout->addWidget(statsAbsencesSummaryLabel);

    leftLayout->addWidget(gradesBox, 1);
    leftLayout->addWidget(absBox, 1);

    splitter->addWidget(leftPane);

    statsDetailWidget = new QWidget(splitter);
    auto* detailLayout = new QVBoxLayout(statsDetailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(12);

    auto* detailHeader = new QFrame(statsDetailWidget);
    detailHeader->setFrameShape(QFrame::StyledPanel);
    detailHeader->setStyleSheet("QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}"
                                "QLabel{color: palette(Text); background: transparent;}");
    auto* dh = new QHBoxLayout(detailHeader);
    dh->setContentsMargins(14, 10, 14, 10);
    dh->setSpacing(10);

    statsDetailTitleLabel = new QLabel("Выберите студента слева", detailHeader);
    statsDetailTitleLabel->setStyleSheet("font-weight: 900; font-size: 14px; color: palette(WindowText);");
    dh->addWidget(statsDetailTitleLabel, 1);

    auto* hint = new QLabel("детали", detailHeader);
    hint->setStyleSheet(UiStyle::badgeNeutralStyle());
    hint->setMinimumHeight(22);
    hint->setAlignment(Qt::AlignCenter);
    dh->addWidget(hint, 0, Qt::AlignRight);

    detailLayout->addWidget(detailHeader);

    auto* detailGradesBox = new QGroupBox("Все оценки студента", statsDetailWidget);
    auto* dgLayout = new QVBoxLayout(detailGradesBox);
    dgLayout->setContentsMargins(10, 8, 10, 8);
    dgLayout->setSpacing(8);

    statsDetailGradesTable = new QTableWidget(detailGradesBox);
    UiStyle::applyStandardTableStyle(statsDetailGradesTable);
    statsDetailGradesTable->setColumnCount(7);
    statsDetailGradesTable->setHorizontalHeaderLabels({"Дата", "Время", "Предмет", "Оценка", "Занятие", "Ауд.", "Тип"});
    statsDetailGradesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsDetailGradesTable->horizontalHeader()->setStretchLastSection(true);
    statsDetailGradesTable->setSortingEnabled(true);
    dgLayout->addWidget(statsDetailGradesTable);

    statsDetailGradesSummaryLabel = new QLabel(detailGradesBox);
    UiStyle::makeInfoLabel(statsDetailGradesSummaryLabel);
    dgLayout->addWidget(statsDetailGradesSummaryLabel);

    auto* detailAbsBox = new QGroupBox("Все пропуски студента", statsDetailWidget);
    auto* daLayout = new QVBoxLayout(detailAbsBox);
    daLayout->setContentsMargins(10, 8, 10, 8);
    daLayout->setSpacing(8);

    statsDetailAbsencesTable = new QTableWidget(detailAbsBox);
    UiStyle::applyStandardTableStyle(statsDetailAbsencesTable);
    statsDetailAbsencesTable->setColumnCount(7);
    statsDetailAbsencesTable->setHorizontalHeaderLabels({"Дата", "Время", "Предмет", "Часы", "Тип", "Занятие", "Ауд."});
    statsDetailAbsencesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    statsDetailAbsencesTable->horizontalHeader()->setStretchLastSection(true);
    statsDetailAbsencesTable->setSortingEnabled(true);
    daLayout->addWidget(statsDetailAbsencesTable);

    statsDetailAbsencesSummaryLabel = new QLabel(detailAbsBox);
    UiStyle::makeInfoLabel(statsDetailAbsencesSummaryLabel);
    daLayout->addWidget(statsDetailAbsencesSummaryLabel);

    detailLayout->addWidget(detailGradesBox, 1);
    detailLayout->addWidget(detailAbsBox, 1);
    splitter->addWidget(statsDetailWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    // semesters
    if (db) {
        std::vector<std::pair<int, std::string>> semesters;
        if (db->getAllSemesters(semesters) && !semesters.empty()) {
            for (const auto& s : semesters) {
                statsSemesterCombo->addItem(QString::fromStdString(s.second).isEmpty() ? QString("Семестр %1").arg(s.first)
                                                                                       : QString::fromStdString(s.second),
                                            s.first);
            }
        } else {
            statsSemesterCombo->addItem("Семестр 1", 1);
        }
    }

    statsSubjectCombo->addItem("Все предметы", 0);

    connect(statsGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onStatsGroupChanged);
    connect(statsSemesterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onStatsSemesterChanged);
    connect(statsSubjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onStatsSubjectChanged);
    connect(refresh, &QPushButton::clicked, this, &TeacherWindow::reloadGroupStats);

    connect(statsGradesTable, &QTableWidget::itemSelectionChanged,
            this, &TeacherWindow::onStatsStudentSelected);
    connect(statsAbsencesTable, &QTableWidget::itemSelectionChanged,
            this, &TeacherWindow::onStatsStudentSelected);

    statsGradesSummaryLabel->setText("Оценки: —");
    statsAbsencesSummaryLabel->setText("Пропуски: —");
    statsDetailGradesSummaryLabel->setText("Оценки: —");
    statsDetailAbsencesSummaryLabel->setText("Пропуски: —");
    return root;
}

static QString journalTimelineCardStyle()
{
    return "QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}"
           "QLabel{color: palette(Text); background: transparent;}";
}

static QWidget* buildJournalDayHeader(QWidget* parent, const QString& title)
{
    auto* w = new QWidget(parent);
    auto* row = new QHBoxLayout(w);
    row->setContentsMargins(6, 8, 6, 6);
    row->setSpacing(10);

    auto* label = new QLabel(title.isEmpty() ? QString("—") : title, w);
    label->setStyleSheet("font-weight: 900; font-size: 13px; color: palette(WindowText);");
    row->addWidget(label);

    auto* line = new QFrame(w);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: rgba(120,120,120,0.35);");
    row->addWidget(line, 1);

    return w;
}

static QString journalAccentStripeStyleForLessonType(const QString& lessonType)
{
    if (lessonType.contains("ЛР", Qt::CaseInsensitive)) return "background: rgba(255, 145, 80, 0.85);";
    if (lessonType.contains("ПЗ", Qt::CaseInsensitive)) return "background: rgba(70, 170, 255, 0.85);";
    if (lessonType.contains("ЛК", Qt::CaseInsensitive)) return "background: rgba(170, 120, 255, 0.85);";
    return "background: rgba(120,120,120,0.55);";
}

static QWidget* buildJournalLessonCard(QWidget* parent,
                                      const TeacherWindow::JournalLessonRow& row,
                                      int index,
                                      TeacherWindow* wnd)
{
    auto* card = new QFrame(parent);
    card->setStyleSheet(journalTimelineCardStyle());
    card->setFrameShape(QFrame::StyledPanel);

    auto* outer = new QHBoxLayout(card);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* stripe = new QFrame(card);
    stripe->setFixedWidth(6);
    stripe->setStyleSheet(journalAccentStripeStyleForLessonType(row.lesson.lessonType));
    outer->addWidget(stripe);

    auto* body = new QWidget(card);
    outer->addWidget(body, 1);

    auto* grid = new QGridLayout(body);
    grid->setContentsMargins(14, 12, 14, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);

    auto* title = new QLabel(row.lesson.subjectName.isEmpty() ? QString("—") : row.lesson.subjectName, card);
    title->setStyleSheet("font-weight: 800; font-size: 14px; color: palette(WindowText);");
    grid->addWidget(title, 0, 0, 1, 2);

    auto toDdMm = [](const QString& dateISO) {
        if (dateISO.size() >= 10) {
            return dateISO.mid(8, 2) + "." + dateISO.mid(5, 2);
        }
        return dateISO;
    };

    const QString dateText = row.lesson.dateISO.isEmpty() ? QString("—") : row.lesson.dateISO;
    const QString metaText = QString("%1  •  %2  •  %3")
        .arg(toDdMm(dateText))
        .arg(row.weekdayName.isEmpty() ? QString("—") : row.weekdayName)
        .arg(row.timeText.isEmpty() ? QString("—") : row.timeText);
    auto* meta = new QLabel(metaText, card);
    meta->setStyleSheet("color: palette(mid); font-weight: 600;");
    grid->addWidget(meta, 1, 0, 1, 2);

    auto* right = new QWidget(card);
    auto* rightCol = new QVBoxLayout(right);
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(6);

    auto* numBadge = new QLabel(row.lesson.lessonNumber > 0 ? QString("№%1").arg(row.lesson.lessonNumber) : QString("—"), right);
    numBadge->setAlignment(Qt::AlignCenter);
    numBadge->setMinimumHeight(22);
    numBadge->setStyleSheet(neutralBadgeStyle());
    rightCol->addWidget(numBadge, 0, Qt::AlignRight);

    auto* roomBadge = new QLabel(row.lesson.room.isEmpty() ? QString("—") : row.lesson.room, right);
    roomBadge->setAlignment(Qt::AlignCenter);
    roomBadge->setMinimumHeight(22);
    roomBadge->setStyleSheet(neutralBadgeStyle());
    rightCol->addWidget(roomBadge, 0, Qt::AlignRight);

    if (!row.lesson.lessonType.isEmpty()) {
        auto* typeBadge = new QLabel(row.lesson.lessonType, right);
        typeBadge->setAlignment(Qt::AlignCenter);
        typeBadge->setMinimumHeight(22);
        typeBadge->setStyleSheet(lessonTypeBadgeStyle(row.lesson.lessonType));
        rightCol->addWidget(typeBadge, 0, Qt::AlignRight);
    }
    rightCol->addStretch();
    grid->addWidget(right, 0, 2, 2, 1, Qt::AlignRight | Qt::AlignTop);

    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("journalLessonIndex", index);
    card->installEventFilter(wnd);
    return card;
}

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
    scheduleGroupCombo->setMinimumWidth(160);
    filtersRow->addWidget(scheduleGroupCombo);

    filtersRow->addSpacing(12);
    filtersRow->addWidget(new QLabel("Подгруппа:", root));
    scheduleSubgroupCombo = new QComboBox(root);
    scheduleSubgroupCombo->addItem("Все", 0);
    scheduleSubgroupCombo->addItem("1", 1);
    scheduleSubgroupCombo->addItem("2", 2);
    scheduleSubgroupCombo->setMinimumWidth(120);
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

QWidget* TeacherWindow::buildJournalTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);

    journalPeriodSelector = new PeriodSelectorWidget(db, root);
    layout->addWidget(journalPeriodSelector);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);

    topRow->addWidget(new QLabel("Режим:", root));
    journalModeCombo = new QComboBox(root);
    journalModeCombo->addItem("По паре (из расписания)", 0);
    journalModeCombo->addItem("Свободный режим (пропуски)", 1);
    journalModeCombo->setMinimumWidth(220);
    topRow->addWidget(journalModeCombo);

    topRow->addSpacing(12);

    topRow->addWidget(new QLabel("Семестр:", root));
    journalSemesterCombo = new QComboBox(root);
    journalSemesterCombo->setMinimumWidth(160);
    topRow->addWidget(journalSemesterCombo);

    topRow->addSpacing(12);
    topRow->addWidget(new QLabel("Группа:", root));
    journalGroupCombo = new QComboBox(root);
    journalGroupCombo->setMinimumWidth(180);
    topRow->addWidget(journalGroupCombo);

    topRow->addStretch();
    layout->addLayout(topRow);

    journalModeStack = new QStackedWidget(root);
    layout->addWidget(journalModeStack);

    // ===== Mode 0: by lesson =====
    auto* byLesson = new QWidget(root);
    auto* byLessonLayout = new QVBoxLayout(byLesson);
    byLessonLayout->setContentsMargins(0, 0, 0, 0);

    journalLessonCardFrame = new QFrame(byLesson);
    journalLessonCardFrame->setObjectName("lessonCardFrame");
    journalLessonCardFrame->setFrameShape(QFrame::StyledPanel);
    journalLessonCardFrame->setStyleSheet("#lessonCardFrame{border:1px solid rgba(127,127,127,0.25); border-radius:12px; padding:12px;}");

    auto* cardLayout = new QVBoxLayout(journalLessonCardFrame);
    cardLayout->setContentsMargins(12, 10, 12, 12);
    cardLayout->setSpacing(8);

    journalLessonCardTitle = new QLabel("Выбранная пара", journalLessonCardFrame);
    journalLessonCardTitle->setStyleSheet("font-weight: 700; font-size: 14px; color: palette(WindowText);");
    cardLayout->addWidget(journalLessonCardTitle);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(6);

    auto mkKey = [this](const QString& text) {
        auto* l = new QLabel(text, journalLessonCardFrame);
        l->setStyleSheet("color: palette(mid); font-size: 12px;");
        return l;
    };
    auto mkVal = [this]() {
        auto* l = new QLabel("—", journalLessonCardFrame);
        l->setStyleSheet("color: palette(WindowText); font-size: 13px;");
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };

    auto mkBadgeVal = [this]() {
        auto* l = new QLabel("—", journalLessonCardFrame);
        l->setAlignment(Qt::AlignCenter);
        l->setMinimumHeight(22);
        l->setStyleSheet(neutralBadgeStyle());
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };

    journalLessonCardDate = mkVal();
    journalLessonCardWeekday = mkVal();
    journalLessonCardTime = mkVal();
    journalLessonCardNumber = mkVal();
    journalLessonCardSubject = mkVal();
    journalLessonCardType = mkBadgeVal();
    journalLessonCardRoom = mkBadgeVal();
    journalLessonCardGroup = mkBadgeVal();

    int r = 0;
    grid->addWidget(mkKey("Дата"), r, 0); grid->addWidget(journalLessonCardDate, r, 1);
    grid->addWidget(mkKey("День"), r, 2); grid->addWidget(journalLessonCardWeekday, r, 3);
    ++r;
    grid->addWidget(mkKey("Время"), r, 0); grid->addWidget(journalLessonCardTime, r, 1);
    grid->addWidget(mkKey("№ пары"), r, 2); grid->addWidget(journalLessonCardNumber, r, 3);
    ++r;
    grid->addWidget(mkKey("Предмет"), r, 0); grid->addWidget(journalLessonCardSubject, r, 1, 1, 3);
    ++r;
    grid->addWidget(mkKey("Тип"), r, 0); grid->addWidget(journalLessonCardType, r, 1);
    grid->addWidget(mkKey("Аудитория"), r, 2); grid->addWidget(journalLessonCardRoom, r, 3);
    ++r;
    grid->addWidget(mkKey("Группа"), r, 0); grid->addWidget(journalLessonCardGroup, r, 1, 1, 3);

    cardLayout->addLayout(grid);
    byLessonLayout->addWidget(journalLessonCardFrame);

    auto* tablesRow = new QHBoxLayout();
    tablesRow->setContentsMargins(0, 0, 0, 0);

    journalLessonsScroll = new QScrollArea(byLesson);
    journalLessonsScroll->setWidgetResizable(true);
    journalLessonsScroll->setFrameShape(QFrame::NoFrame);

    journalLessonsContainer = new QWidget(journalLessonsScroll);
    journalLessonsLayout = new QVBoxLayout(journalLessonsContainer);
    journalLessonsLayout->setContentsMargins(10, 10, 10, 10);
    journalLessonsLayout->setSpacing(10);
    journalLessonsLayout->addStretch();
    journalLessonsScroll->setWidget(journalLessonsContainer);

    tablesRow->addWidget(journalLessonsScroll, 2);

    journalStudentsTable = new QTableWidget(byLesson);
    UiStyle::applyStandardTableStyle(journalStudentsTable);
    journalStudentsTable->setColumnCount(2);
    journalStudentsTable->setHorizontalHeaderLabels({"ID", "Студент"});
    journalStudentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    journalStudentsTable->horizontalHeader()->setStretchLastSection(true);
    journalStudentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    journalStudentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    tablesRow->addWidget(journalStudentsTable, 1);

    byLessonLayout->addLayout(tablesRow);

    auto* actionsRow = new QHBoxLayout();
    actionsRow->setContentsMargins(0, 0, 0, 0);
    actionsRow->setSpacing(12);

    auto* gradeBox = new QGroupBox("Оценка", byLesson);
    gradeBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* gradeLayout = new QHBoxLayout(gradeBox);
    gradeLayout->setContentsMargins(10, 8, 10, 10);
    gradeLayout->setSpacing(8);

    gradeLayout->addWidget(new QLabel("Значение (0..10):", gradeBox));
    gradeSpin = new QSpinBox(gradeBox);
    gradeSpin->setRange(0, 10);
    gradeSpin->setValue(5);
    gradeLayout->addWidget(gradeSpin);

    currentGradeLabel = new QLabel("Оценка: —", gradeBox);
    UiStyle::makeInfoLabel(currentGradeLabel);
    gradeLayout->addWidget(currentGradeLabel);

    gradeLayout->addStretch();

    saveGradeButton = new QPushButton("Сохранить", gradeBox);
    gradeLayout->addWidget(saveGradeButton);

    deleteGradeButton = new QPushButton("Удалить", gradeBox);
    gradeLayout->addWidget(deleteGradeButton);

    auto* absenceBox = new QGroupBox("Пропуск", byLesson);
    absenceBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* absenceLayout = new QHBoxLayout(absenceBox);
    absenceLayout->setContentsMargins(10, 8, 10, 10);
    absenceLayout->setSpacing(8);

    absenceLayout->addWidget(new QLabel("Часы:", absenceBox));
    absenceHoursSpin = new QSpinBox(absenceBox);
    absenceHoursSpin->setRange(1, 8);
    absenceHoursSpin->setValue(1);
    absenceLayout->addWidget(absenceHoursSpin);

    absenceTypeCombo = new QComboBox(absenceBox);
    absenceTypeCombo->addItem("Уважительный", "excused");
    absenceTypeCombo->addItem("Неуважительный", "unexcused");
    absenceLayout->addWidget(absenceTypeCombo);

    currentAbsenceLabel = new QLabel("Пропуск: —", absenceBox);
    UiStyle::makeInfoLabel(currentAbsenceLabel);
    absenceLayout->addWidget(currentAbsenceLabel);

    absenceLayout->addStretch();

    saveAbsenceButton = new QPushButton("Сохранить", absenceBox);
    absenceLayout->addWidget(saveAbsenceButton);

    deleteAbsenceButton = new QPushButton("Удалить", absenceBox);
    absenceLayout->addWidget(deleteAbsenceButton);

    actionsRow->addWidget(gradeBox, 1);
    actionsRow->addWidget(absenceBox, 1);
    byLessonLayout->addLayout(actionsRow);

    journalModeStack->addWidget(byLesson);

    // ===== Mode 1: free absences =====
    auto* freeMode = new QWidget(root);
    auto* freeLayout = new QVBoxLayout(freeMode);
    freeLayout->setContentsMargins(0, 0, 0, 0);

    auto* freeInfo = new QLabel("Свободный режим позволяет отмечать пропуски для любого студента любой группы.", freeMode);
    freeInfo->setWordWrap(true);
    UiStyle::makeInfoLabel(freeInfo);
    freeLayout->addWidget(freeInfo);

    auto* freeRow1 = new QHBoxLayout();
    freeRow1->setContentsMargins(0, 0, 0, 0);
    freeRow1->addWidget(new QLabel("Группа:", freeMode));
    freeGroupCombo = new QComboBox(freeMode);
    freeGroupCombo->setMinimumWidth(220);
    freeRow1->addWidget(freeGroupCombo);
    freeRow1->addSpacing(12);
    freeRow1->addWidget(new QLabel("Студент:", freeMode));
    freeStudentCombo = new QComboBox(freeMode);
    freeStudentCombo->setMinimumWidth(260);
    freeRow1->addWidget(freeStudentCombo);
    freeRow1->addStretch();
    freeLayout->addLayout(freeRow1);

    auto* freeRow2 = new QHBoxLayout();
    freeRow2->setContentsMargins(0, 0, 0, 0);
    freeRow2->addWidget(new QLabel("Предмет:", freeMode));
    freeSubjectCombo = new QComboBox(freeMode);
    freeSubjectCombo->setMinimumWidth(280);
    freeRow2->addWidget(freeSubjectCombo);
    freeRow2->addSpacing(12);
    freeRow2->addWidget(new QLabel("Дата:", freeMode));
    freeDateEdit = new QDateEdit(QDate::currentDate(), freeMode);
    freeDateEdit->setDisplayFormat("dd.MM.yyyy");
    freeDateEdit->setCalendarPopup(true);
    freeRow2->addWidget(freeDateEdit);
    freeRow2->addStretch();
    freeLayout->addLayout(freeRow2);

    auto* freeRow3 = new QHBoxLayout();
    freeRow3->setContentsMargins(0, 0, 0, 0);
    freeRow3->addWidget(new QLabel("Часы:", freeMode));
    freeHoursSpin = new QSpinBox(freeMode);
    freeHoursSpin->setRange(1, 8);
    freeHoursSpin->setValue(1);
    freeRow3->addWidget(freeHoursSpin);
    freeRow3->addSpacing(12);
    freeRow3->addWidget(new QLabel("Тип:", freeMode));
    freeTypeCombo = new QComboBox(freeMode);
    freeTypeCombo->addItem("Уважительный", "excused");
    freeTypeCombo->addItem("Неуважительный", "unexcused");
    freeRow3->addWidget(freeTypeCombo);
    freeSaveAbsenceButton = new QPushButton("Сохранить пропуск", freeMode);
    freeRow3->addWidget(freeSaveAbsenceButton);
    freeRow3->addStretch();
    freeLayout->addLayout(freeRow3);

    journalModeStack->addWidget(freeMode);

    connect(journalModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onJournalModeChanged);

    connect(journalPeriodSelector, &PeriodSelectorWidget::selectionChanged,
            this, &TeacherWindow::onJournalPeriodChanged);
    connect(journalGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onJournalGroupChanged);
    connect(journalSemesterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        reloadJournalLessonsForSelectedStudent();
    });

    connect(journalStudentsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TeacherWindow::onJournalStudentSelectionChanged);

    connect(saveGradeButton, &QPushButton::clicked, this, &TeacherWindow::openGradeDialog);
    connect(saveAbsenceButton, &QPushButton::clicked, this, &TeacherWindow::openAbsenceDialog);
    connect(deleteGradeButton, &QPushButton::clicked, this, &TeacherWindow::onDeleteGrade);
    connect(deleteAbsenceButton, &QPushButton::clicked, this, &TeacherWindow::onDeleteAbsence);

    connect(freeGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onFreeGroupChanged);
    connect(freeSaveAbsenceButton, &QPushButton::clicked, this, &TeacherWindow::onFreeSaveAbsence);

    // semesters
    {
        std::vector<std::pair<int, std::string>> semesters;
        if (db && db->getAllSemesters(semesters) && !semesters.empty()) {
            for (const auto& s : semesters) {
                journalSemesterCombo->addItem(QString::fromStdString(s.second).isEmpty() ? QString("Семестр %1").arg(s.first)
                                                                                          : QString::fromStdString(s.second),
                                              s.first);
            }
        } else {
            journalSemesterCombo->addItem("Семестр 1", 1);
        }
    }

    onJournalPeriodChanged(journalPeriodSelector->currentSelection());

    // free-mode data
    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            freeGroupCombo->addItem("Выберите группу", 0);
            for (const auto& g : groups) {
                freeGroupCombo->addItem(QString::fromStdString(g.second), g.first);
            }
        }

        freeSubjectCombo->addItem("Сначала выберите группу", 0);
    }

    onJournalModeChanged(journalModeCombo->currentIndex());
    return root;
}

void TeacherWindow::onJournalModeChanged(int)
{
    if (!journalModeStack || !journalModeCombo) return;
    const int mode = journalModeCombo->currentData().toInt();
    journalModeStack->setCurrentIndex(mode);
}

void TeacherWindow::onFreeGroupChanged(int)
{
    if (!db || !freeGroupCombo || !freeStudentCombo) return;

    freeStudentCombo->clear();
    const int groupId = freeGroupCombo->currentData().toInt();
    if (groupId <= 0) {
        freeStudentCombo->addItem("Выберите группу", 0);
        return;
    }

    std::vector<std::pair<int, std::string>> students;
    if (!db->getStudentsOfGroup(groupId, students)) {
        freeStudentCombo->addItem("Не удалось загрузить", 0);
        return;
    }

    freeStudentCombo->addItem("Выберите студента", 0);
    for (const auto& st : students) {
        freeStudentCombo->addItem(QString::fromStdString(st.second), st.first);
    }

    if (freeSubjectCombo) {
        freeSubjectCombo->clear();
        std::vector<std::pair<int, std::string>> subjects;
        if (db->getSubjectsForTeacherInGroupSchedule(teacherId, groupId, subjects) && !subjects.empty()) {
            freeSubjectCombo->addItem("Выберите предмет", 0);
            for (const auto& s : subjects) {
                freeSubjectCombo->addItem(QString::fromStdString(s.second), s.first);
            }
        } else {
            freeSubjectCombo->addItem("Нет предметов (по расписанию)", 0);
        }
    }
}

void TeacherWindow::openGradeDialog()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару.");
        return;
    }
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }

    if (selectedLesson.lessonType.contains("ЛК")) {
        QMessageBox::information(this, "Журнал", "На лекции оценка не выставляется.");
        return;
    }

    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const QString studentName = journalStudentsTable->item(sRow, 1) ? journalStudentsTable->item(sRow, 1)->text() : QString();
    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (studentId <= 0 || semesterId <= 0) return;

    // conflict with absence
    int absenceId = 0;
    if (!db->findAbsenceId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), absenceId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при проверке пропусков.");
        return;
    }
    if (absenceId > 0) {
        QMessageBox::warning(this, "Журнал", "Нельзя поставить оценку: уже есть пропуск на эту дату/предмет.");
        return;
    }

    // existing grade
    int currentValue = gradeSpin ? gradeSpin->value() : 5;
    int gradeId = 0;
    if (db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId) && gradeId > 0) {
        int v = 0;
        std::string d;
        std::string t;
        if (db->getGradeById(gradeId, v, d, t)) currentValue = v;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Оценка");
    auto* root = new QVBoxLayout(&dlg);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignTop);

    auto* info = new QLabel(QString("<b>%1</b><br>%2, %3")
                            .arg(studentName.isEmpty() ? QString("Студент %1").arg(studentId) : studentName)
                            .arg(selectedLesson.subjectName)
                            .arg(formatDdMm(selectedLesson.dateISO)), &dlg);
    info->setWordWrap(true);
    root->addWidget(info);

    auto* valueSpin = new QSpinBox(&dlg);
    valueSpin->setRange(0, 10);
    valueSpin->setValue(currentValue);
    form->addRow("Значение:", valueSpin);

    root->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Сохранить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const int gradeValue = valueSpin->value();
    if (gradeValue < 0 || gradeValue > 10) {
        QMessageBox::warning(this, "Журнал", "Оценка должна быть в диапазоне 0..10.");
        return;
    }

    if (!db->upsertGradeByKey(studentId, selectedLesson.subjectId, semesterId, gradeValue,
                              selectedLesson.dateISO.toStdString(), "")) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить оценку.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Оценка сохранена.");
    refreshJournalCurrentValues();
}

void TeacherWindow::openAbsenceDialog()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару.");
        return;
    }
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }

    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const QString studentName = journalStudentsTable->item(sRow, 1) ? journalStudentsTable->item(sRow, 1)->text() : QString();
    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (studentId <= 0 || semesterId <= 0) return;

    // conflict with grade
    int gradeId = 0;
    if (!db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при проверке оценок.");
        return;
    }
    if (gradeId > 0) {
        QMessageBox::warning(this, "Журнал", "Нельзя отметить пропуск: уже есть оценка на эту дату/предмет.");
        return;
    }

    int currentHours = absenceHoursSpin ? absenceHoursSpin->value() : 1;
    QString currentType = absenceTypeCombo ? absenceTypeCombo->currentData().toString() : "excused";
    int absenceId = 0;
    if (db->findAbsenceId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), absenceId) && absenceId > 0) {
        int h = 0;
        std::string d;
        std::string t;
        if (db->getAbsenceById(absenceId, h, d, t)) {
            currentHours = h;
            currentType = QString::fromStdString(t);
        }
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Пропуск");
    auto* root = new QVBoxLayout(&dlg);

    auto* info = new QLabel(QString("<b>%1</b><br>%2, %3")
                            .arg(studentName.isEmpty() ? QString("Студент %1").arg(studentId) : studentName)
                            .arg(selectedLesson.subjectName)
                            .arg(formatDdMm(selectedLesson.dateISO)), &dlg);
    info->setWordWrap(true);
    root->addWidget(info);

    auto* form = new QFormLayout();

    auto* hoursSpin = new QSpinBox(&dlg);
    hoursSpin->setRange(1, 8);
    hoursSpin->setValue(currentHours);
    form->addRow("Часы:", hoursSpin);

    auto* typeCombo = new QComboBox(&dlg);
    typeCombo->addItem("Уважительный", "excused");
    typeCombo->addItem("Неуважительный", "unexcused");
    const int idx = typeCombo->findData(currentType);
    if (idx >= 0) typeCombo->setCurrentIndex(idx);
    form->addRow("Тип:", typeCombo);

    root->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText("Сохранить");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const int hours = hoursSpin->value();
    if (hours <= 0 || hours > 8) {
        QMessageBox::warning(this, "Журнал", "Часы пропуска должны быть в диапазоне 1..8.");
        return;
    }

    const QString type = typeCombo->currentData().toString();
    if (type != "excused" && type != "unexcused") {
        QMessageBox::warning(this, "Журнал", "Некорректный тип пропуска.");
        return;
    }

    if (!db->upsertAbsenceByKey(studentId, selectedLesson.subjectId, semesterId, hours,
                                selectedLesson.dateISO.toStdString(), type.toStdString())) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить пропуск.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Пропуск сохранён.");
    refreshJournalCurrentValues();
}

void TeacherWindow::onFreeSaveAbsence()
{
    if (!db) return;

    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (semesterId <= 0) {
        QMessageBox::warning(this, "Журнал", "Некорректный семестр.");
        return;
    }

    const int groupId = freeGroupCombo ? freeGroupCombo->currentData().toInt() : 0;
    if (groupId <= 0) {
        QMessageBox::warning(this, "Журнал", "Выберите группу.");
        return;
    }

    const int studentId = freeStudentCombo ? freeStudentCombo->currentData().toInt() : 0;
    if (studentId <= 0) {
        QMessageBox::warning(this, "Журнал", "Выберите студента.");
        return;
    }

    const int subjectId = freeSubjectCombo ? freeSubjectCombo->currentData().toInt() : 0;
    if (subjectId <= 0) {
        QMessageBox::warning(this, "Журнал", "Выберите предмет.");
        return;
    }

    const QString dateISO = freeDateEdit ? freeDateEdit->date().toString("yyyy-MM-dd") : QString();
    if (dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Выберите дату.");
        return;
    }

    const int hours = freeHoursSpin ? freeHoursSpin->value() : 0;
    if (hours <= 0 || hours > 8) {
        QMessageBox::warning(this, "Журнал", "Часы пропуска должны быть в диапазоне 1..8.");
        return;
    }

    const QString type = freeTypeCombo ? freeTypeCombo->currentData().toString() : "excused";
    if (type != "excused" && type != "unexcused") {
        QMessageBox::warning(this, "Журнал", "Некорректный тип пропуска.");
        return;
    }

    // conflict with grade
    int gradeId = 0;
    if (!db->findGradeId(studentId, subjectId, semesterId, dateISO.toStdString(), gradeId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при проверке оценок.");
        return;
    }
    if (gradeId > 0) {
        QMessageBox::warning(this, "Журнал", "Нельзя отметить пропуск: уже есть оценка на эту дату/предмет.");
        return;
    }

    if (!db->upsertAbsenceByKey(studentId, subjectId, semesterId, hours, dateISO.toStdString(), type.toStdString())) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить пропуск.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Пропуск сохранён.");
}

void TeacherWindow::loadTeacherGroups()
{
    if (!db) return;
    std::vector<std::pair<int, std::string>> groups;
    if (!db->getGroupsForTeacher(teacherId, groups)) return;

    scheduleGroupCombo->blockSignals(true);
    journalGroupCombo->blockSignals(true);
    if (statsGroupCombo) statsGroupCombo->blockSignals(true);
    scheduleGroupCombo->clear();
    journalGroupCombo->clear();
    if (statsGroupCombo) statsGroupCombo->clear();
    scheduleGroupCombo->addItem("Все группы", 0);
    journalGroupCombo->addItem("Выберите группу", 0);
    if (statsGroupCombo) statsGroupCombo->addItem("Выберите группу", 0);

    for (const auto& g : groups) {
        const int id = g.first;
        const QString name = QString::fromStdString(g.second);
        scheduleGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
        journalGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
        if (statsGroupCombo) statsGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
    }

    scheduleGroupCombo->blockSignals(false);
    journalGroupCombo->blockSignals(false);
    if (statsGroupCombo) statsGroupCombo->blockSignals(false);

    // default: all groups
    reloadSchedule();

    reloadGroupStatsSubjects();
    reloadGroupStats();
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
    if (!db || !statsGroupCombo || !statsSemesterCombo || !statsSubjectCombo) return;
    if (!statsGradesTable || !statsAbsencesTable || !statsGradesSummaryLabel || !statsAbsencesSummaryLabel) return;

    const int groupId = statsGroupCombo->currentData().toInt();
    const int semesterId = statsSemesterCombo->currentData().toInt();
    const int subjectId = statsSubjectCombo->currentData().toInt();
    const QString subjectName = statsSubjectCombo->currentText();

    statsGradesTable->setRowCount(0);
    statsAbsencesTable->setRowCount(0);

    if (groupId <= 0 || semesterId <= 0) {
        statsGradesSummaryLabel->setText("Оценки: выберите группу и семестр");
        statsAbsencesSummaryLabel->setText("Пропуски: выберите группу и семестр");
        return;
    }

    std::vector<std::pair<int, std::string>> students;
    if (!db->getStudentsOfGroup(groupId, students)) {
        statsGradesSummaryLabel->setText("Оценки: не удалось загрузить студентов");
        statsAbsencesSummaryLabel->setText("Пропуски: не удалось загрузить студентов");
        return;
    }

    double groupSum = 0.0;
    int groupCount = 0;
    int groupAbsTotal = 0;
    int groupAbsUnexc = 0;

    for (const auto& st : students) {
        const int studentId = st.first;
        const QString studentName = QString::fromStdString(st.second);

        // ===== grades =====
        double sum = 0.0;
        int cnt = 0;
        if (subjectId > 0) {
            std::vector<std::tuple<int, std::string, std::string>> grades;
            if (db->getStudentSubjectGrades(studentId, subjectId, semesterId, grades)) {
                for (const auto& g : grades) {
                    sum += std::get<0>(g);
                    ++cnt;
                }
            }
        } else {
            std::vector<std::tuple<std::string, int, std::string, std::string>> grades;
            if (db->getStudentGradesForSemester(studentId, semesterId, grades)) {
                for (const auto& g : grades) {
                    sum += std::get<1>(g);
                    ++cnt;
                }
            }
        }

        const double avg = (cnt > 0) ? (sum / cnt) : 0.0;
        groupSum += sum;
        groupCount += cnt;

        const int gRow = statsGradesTable->rowCount();
        statsGradesTable->insertRow(gRow);
        auto* nameItem = new QTableWidgetItem(studentName.isEmpty() ? QString("ID %1").arg(studentId) : studentName);
        nameItem->setData(Qt::UserRole, studentId);
        statsGradesTable->setItem(gRow, 0, nameItem);

        if (cnt > 0) {
            auto* badge = new QLabel(QString::number(avg, 'f', 2), statsGradesTable);
            badge->setAlignment(Qt::AlignCenter);
            badge->setMinimumHeight(22);
            badge->setStyleSheet(UiStyle::badgeGradeStyle(qRound(avg)));
            statsGradesTable->setCellWidget(gRow, 1, badge);
        } else {
            auto* none = new QTableWidgetItem("—");
            none->setTextAlignment(Qt::AlignCenter);
            statsGradesTable->setItem(gRow, 1, none);
        }

        auto* cntItem = new QTableWidgetItem(cnt > 0 ? QString::number(cnt) : "0");
        cntItem->setTextAlignment(Qt::AlignCenter);
        statsGradesTable->setItem(gRow, 2, cntItem);

        // ===== absences =====
        int totalH = 0;
        int unexcH = 0;
        std::vector<std::tuple<std::string, int, std::string, std::string>> abs;
        if (db->getStudentAbsencesForSemester(studentId, semesterId, abs)) {
            for (const auto& a : abs) {
                const QString subj = QString::fromStdString(std::get<0>(a));
                const int hours = std::get<1>(a);
                const QString type = QString::fromStdString(std::get<3>(a));

                if (subjectId > 0 && subj != subjectName) continue;
                totalH += hours;
                if (type == "unexcused") unexcH += hours;
            }
        }
        groupAbsTotal += totalH;
        groupAbsUnexc += unexcH;

        const int aRow = statsAbsencesTable->rowCount();
        statsAbsencesTable->insertRow(aRow);
        auto* aNameItem = new QTableWidgetItem(studentName.isEmpty() ? QString("ID %1").arg(studentId) : studentName);
        aNameItem->setData(Qt::UserRole, studentId);
        statsAbsencesTable->setItem(aRow, 0, aNameItem);

        auto* tItem = new QTableWidgetItem(QString::number(totalH));
        tItem->setTextAlignment(Qt::AlignCenter);
        statsAbsencesTable->setItem(aRow, 1, tItem);

        if (unexcH > 0) {
            auto* badge = new QLabel(QString::number(unexcH), statsAbsencesTable);
            badge->setAlignment(Qt::AlignCenter);
            badge->setMinimumHeight(22);
            badge->setStyleSheet(UiStyle::badgeAbsenceStyle(false));
            statsAbsencesTable->setCellWidget(aRow, 2, badge);
        } else {
            auto* uItem = new QTableWidgetItem("0");
            uItem->setTextAlignment(Qt::AlignCenter);
            statsAbsencesTable->setItem(aRow, 2, uItem);
        }
    }

    const double groupAvg = (groupCount > 0) ? (groupSum / groupCount) : 0.0;
    statsGradesSummaryLabel->setText(QString("Оценки: средний по группе %1 (записей: %2)")
                                     .arg(groupCount > 0 ? QString::number(groupAvg, 'f', 2) : "—")
                                     .arg(groupCount));

    statsAbsencesSummaryLabel->setText(QString("Пропуски: всего %1 ч, неуважительных %2 ч")
                                       .arg(groupAbsTotal)
                                       .arg(groupAbsUnexc));

    statsGradesTable->sortItems(0, Qt::AscendingOrder);
    statsAbsencesTable->sortItems(1, Qt::DescendingOrder);
}

void TeacherWindow::onStatsGroupChanged(int)
{
    reloadGroupStatsSubjects();
    reloadGroupStats();
}

void TeacherWindow::onStatsSemesterChanged(int)
{
    reloadGroupStats();
}

void TeacherWindow::onStatsSubjectChanged(int)
{
    reloadGroupStats();
}

QString TeacherWindow::formatDdMm(const QString& dateISO) const
{
    if (dateISO.size() >= 10) {
        return dateISO.mid(8, 2) + "." + dateISO.mid(5, 2);
    }
    return dateISO;
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

    if (selection.mode == WeekSelection::ByDate) {
        outResolvedWeekId = db->getWeekIdByDate(selection.selectedDate.toStdString());
        if (outResolvedWeekId <= 0) {
            outErrorText = "Для выбранной даты нет данных о цикле недель (cycleweeks).";
            return false;
        }
        outWeekOfCycle = db->getWeekOfCycleByWeekId(outResolvedWeekId);
        if (outWeekOfCycle <= 0) outWeekOfCycle = 1;
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

void TeacherWindow::onJournalPeriodChanged(const WeekSelection&)
{
    reloadJournalLessonsForSelectedStudent();
}

void TeacherWindow::onJournalGroupChanged(int)
{
    reloadJournalStudents();
    reloadJournalLessonsForSelectedStudent();
}

void TeacherWindow::reloadJournalStudents()
{
    if (!journalStudentsTable || !journalGroupCombo) return;

    journalStudentsTable->setRowCount(0);
    const int groupId = journalGroupCombo->currentData().toInt();
    if (groupId <= 0 || !db) return;

    std::vector<std::pair<int, std::string>> students;
    if (!db->getStudentsOfGroup(groupId, students)) return;

    for (const auto& st : students) {
        const int row = journalStudentsTable->rowCount();
        journalStudentsTable->insertRow(row);
        journalStudentsTable->setItem(row, 0, new QTableWidgetItem(QString::number(st.first)));
        journalStudentsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(st.second)));
    }
}

void TeacherWindow::reloadJournalLessonsForSelectedStudent()
{
    if (!journalLessonsLayout || !journalLessonsContainer || !journalPeriodSelector || !journalGroupCombo) return;

    while (journalLessonsLayout->count() > 0) {
        QLayoutItem* it = journalLessonsLayout->takeAt(0);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }
    journalLessonsLayout->addStretch();

    journalLessonRows.clear();
    selectedLesson = {};

    const int groupId = journalGroupCombo->currentData().toInt();
    if (groupId <= 0 || !db) return;

    int weekId = 0;
    int weekOfCycle = 1;
    QString err;
    if (!resolveWeekSelection(journalPeriodSelector->currentSelection(), weekId, weekOfCycle, err)) {
        return;
    }

    // if student is selected - use subgroup filter from student profile, else show all
    int subgroupFilter = 0;
    const auto sel = journalStudentsTable->selectionModel() ? journalStudentsTable->selectionModel()->selectedRows() : QModelIndexList{};
    if (!sel.isEmpty()) {
        const int row = sel.front().row();
        const int studentId = journalStudentsTable->item(row, 0)->text().toInt();
        int sgGroup = 0;
        int sg = 0;
        if (db->getStudentGroupAndSubgroup(studentId, sgGroup, sg) && sgGroup == groupId) {
            subgroupFilter = sg;
        }
    }

    std::vector<std::tuple<int,int,int,int,int,std::string,std::string,std::string>> rows;
    if (!db->getScheduleForTeacherGroupWeekWithRoom(teacherId, groupId, weekOfCycle, subgroupFilter, rows) || rows.empty()) {
        return;
    }

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    for (const auto& r : rows) {
        const int scheduleId = std::get<0>(r);
        const int subjectId = std::get<1>(r);
        const int weekday = std::get<2>(r);
        const int lessonNumber = std::get<3>(r);
        const int subgroup = std::get<4>(r);
        const QString subjectName = QString::fromStdString(std::get<5>(r));
        const QString room = QString::fromStdString(std::get<6>(r));
        const QString lessonType = QString::fromStdString(std::get<7>(r));

        std::string dateISO;
        if (weekId > 0) db->getDateForWeekdayByWeekId(weekId, weekday, dateISO);
        else db->getDateForWeekday(weekOfCycle, weekday, dateISO);
        const QString qISO = QString::fromStdString(dateISO);

        JournalLessonRow rr;
        rr.lesson.valid = true;
        rr.lesson.scheduleId = scheduleId;
        rr.lesson.subjectId = subjectId;
        rr.lesson.weekday = weekday;
        rr.lesson.subgroup = subgroup;
        rr.lesson.subjectName = subjectName;
        rr.lesson.room = room;
        rr.lesson.lessonType = lessonType;
        rr.lesson.dateISO = qISO;
        rr.lesson.lessonNumber = lessonNumber;
        rr.weekdayName = (weekday >= 0 && weekday < dayNames.size()) ? dayNames[weekday] : QString();
        rr.timeText = (lessonNumber >= 1 && lessonNumber <= 6) ? pairTimes[lessonNumber - 1] : QString();

        journalLessonRows.push_back(rr);
    }

    std::sort(journalLessonRows.begin(), journalLessonRows.end(), [](const JournalLessonRow& a, const JournalLessonRow& b) {
        if (a.lesson.dateISO != b.lesson.dateISO) return a.lesson.dateISO < b.lesson.dateISO;
        return a.lesson.lessonNumber < b.lesson.lessonNumber;
    });

    QString lastDate;
    for (int i = 0; i < static_cast<int>(journalLessonRows.size()); ++i) {
        const auto& rr = journalLessonRows[i];
        if (rr.lesson.dateISO != lastDate) {
            const QString header = QString("%1  •  %2")
                .arg(rr.weekdayName.isEmpty() ? QString("—") : rr.weekdayName)
                .arg(rr.lesson.dateISO.isEmpty() ? QString("—") : formatDdMm(rr.lesson.dateISO));
            journalLessonsLayout->insertWidget(journalLessonsLayout->count() - 1, buildJournalDayHeader(journalLessonsContainer, header));
            lastDate = rr.lesson.dateISO;
        }
        journalLessonsLayout->insertWidget(journalLessonsLayout->count() - 1, buildJournalLessonCard(journalLessonsContainer, rr, i, this));
    }
}

bool TeacherWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event && event->type() == QEvent::MouseButtonRelease) {
        if (auto* w = qobject_cast<QWidget*>(watched)) {
            if (w->property("journalLessonIndex").isValid()) {
                const int idx = w->property("journalLessonIndex").toInt();
                onJournalLessonCardClicked(idx);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void TeacherWindow::onJournalLessonCardClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(journalLessonRows.size())) return;
    selectedLesson = journalLessonRows[index].lesson;

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    const QString dateText = selectedLesson.dateISO.isEmpty() ? "—" : formatDdMm(selectedLesson.dateISO);
    const QString weekdayText = (selectedLesson.weekday >= 0 && selectedLesson.weekday < dayNames.size()) ? dayNames[selectedLesson.weekday] : "—";
    const QString timeText = (selectedLesson.lessonNumber >= 1 && selectedLesson.lessonNumber <= 6) ? pairTimes[selectedLesson.lessonNumber - 1] : "—";

    QString groupText = journalGroupCombo ? journalGroupCombo->currentText() : QString();
    if (selectedLesson.subgroup == 1) groupText += " / ПГ1";
    else if (selectedLesson.subgroup == 2) groupText += " / ПГ2";
    if (groupText.isEmpty()) groupText = "—";

    if (journalLessonCardDate) journalLessonCardDate->setText(dateText);
    if (journalLessonCardWeekday) journalLessonCardWeekday->setText(weekdayText);
    if (journalLessonCardTime) journalLessonCardTime->setText(timeText);
    if (journalLessonCardNumber) journalLessonCardNumber->setText(selectedLesson.lessonNumber > 0 ? QString::number(selectedLesson.lessonNumber) : "—");
    if (journalLessonCardSubject) journalLessonCardSubject->setText(selectedLesson.subjectName.isEmpty() ? "—" : selectedLesson.subjectName);
    if (journalLessonCardType) {
        const QString t = selectedLesson.lessonType.isEmpty() ? "—" : selectedLesson.lessonType;
        journalLessonCardType->setText(t);
        journalLessonCardType->setStyleSheet(selectedLesson.lessonType.isEmpty() ? neutralBadgeStyle() : lessonTypeBadgeStyle(selectedLesson.lessonType));
    }
    if (journalLessonCardRoom) {
        journalLessonCardRoom->setText(selectedLesson.room.isEmpty() ? "—" : selectedLesson.room);
        journalLessonCardRoom->setStyleSheet(neutralBadgeStyle());
    }
    if (journalLessonCardGroup) {
        journalLessonCardGroup->setText(groupText);
        journalLessonCardGroup->setStyleSheet(neutralBadgeStyle());
    }

    if (saveGradeButton) {
        saveGradeButton->setEnabled(!selectedLesson.lessonType.contains("ЛК"));
    }

    refreshJournalCurrentValues();
}

void TeacherWindow::onJournalStudentSelectionChanged()
{
    reloadJournalLessonsForSelectedStudent();
    refreshJournalCurrentValues();
}

void TeacherWindow::refreshJournalCurrentValues()
{
    if (!db) return;
    if (!currentGradeLabel || !currentAbsenceLabel) return;

    currentGradeLabel->setText("Оценка: —");
    currentAbsenceLabel->setText("Пропуск: —");
    if (deleteGradeButton) deleteGradeButton->setEnabled(false);
    if (deleteAbsenceButton) deleteAbsenceButton->setEnabled(false);

    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) return;
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;

    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (studentId <= 0 || semesterId <= 0) return;

    int gradeId = 0;
    if (db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId) && gradeId > 0) {
        int value = 0;
        std::string date;
        std::string type;
        if (db->getGradeById(gradeId, value, date, type)) {
            currentGradeLabel->setText(QString("Оценка: %1").arg(value));
            if (gradeSpin) gradeSpin->setValue(value);
            if (deleteGradeButton) deleteGradeButton->setEnabled(true);
        }
    }

    int absenceId = 0;
    if (db->findAbsenceId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), absenceId) && absenceId > 0) {
        int hours = 0;
        std::string date;
        std::string type;
        if (db->getAbsenceById(absenceId, hours, date, type)) {
            const QString qt = QString::fromStdString(type);
            currentAbsenceLabel->setText(QString("Пропуск: %1 ч (%2)")
                                         .arg(hours)
                                         .arg(qt == "unexcused" ? "неуваж" : "уваж"));
            if (absenceHoursSpin) absenceHoursSpin->setValue(hours);
            if (absenceTypeCombo) {
                const int idx = absenceTypeCombo->findData(qt);
                if (idx >= 0) absenceTypeCombo->setCurrentIndex(idx);
            }
            if (deleteAbsenceButton) deleteAbsenceButton->setEnabled(true);
        }
    }
}

void TeacherWindow::onSaveGrade()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару в таблице слева.");
        return;
    }

    if (selectedLesson.lessonType.contains("ЛК")) {
        QMessageBox::warning(this, "Журнал", "Нельзя выставлять оценку за лекцию (ЛК).");
        return;
    }

    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }
    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();

    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (semesterId <= 0) {
        QMessageBox::warning(this, "Журнал", "Некорректный семестр.");
        return;
    }

    // conflict: absence exists on same date+subject
    int absenceId = 0;
    if (!db->findAbsenceId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), absenceId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при проверке пропусков.");
        return;
    }
    if (absenceId > 0) {
        QMessageBox::warning(this, "Журнал", "Нельзя поставить оценку: уже отмечен пропуск на эту дату/предмет.");
        return;
    }

    const int gradeValue = gradeSpin ? gradeSpin->value() : 0;
    if (gradeValue < 0 || gradeValue > 10) {
        QMessageBox::warning(this, "Журнал", "Оценка должна быть в диапазоне 0..10.");
        return;
    }

    if (!db->upsertGradeByKey(studentId, selectedLesson.subjectId, semesterId, gradeValue,
                              selectedLesson.dateISO.toStdString(), /*gradeType*/"")) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить оценку.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Оценка сохранена.");
    refreshJournalCurrentValues();
}

void TeacherWindow::onDeleteGrade()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару.");
        return;
    }
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }
    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (semesterId <= 0 || studentId <= 0) return;

    int gradeId = 0;
    if (!db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при поиске оценки.");
        return;
    }
    if (gradeId <= 0) {
        QMessageBox::information(this, "Журнал", "Оценка не найдена.");
        refreshJournalCurrentValues();
        return;
    }

    if (QMessageBox::question(this, "Журнал", "Удалить оценку?") != QMessageBox::Yes) {
        return;
    }

    if (!db->deleteGrade(gradeId)) {
        QMessageBox::critical(this, "Журнал", "Не удалось удалить оценку.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Оценка удалена.");
    refreshJournalCurrentValues();
}

void TeacherWindow::onSaveAbsence()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару в таблице слева.");
        return;
    }

    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }
    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();

    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (semesterId <= 0) {
        QMessageBox::warning(this, "Журнал", "Некорректный семестр.");
        return;
    }

    // conflict: grade exists on same date+subject
    int gradeId = 0;
    if (!db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при проверке оценок.");
        return;
    }
    if (gradeId > 0) {
        QMessageBox::warning(this, "Журнал", "Нельзя отметить пропуск: уже есть оценка на эту дату/предмет.");
        return;
    }

    const int hours = absenceHoursSpin ? absenceHoursSpin->value() : 0;
    if (hours <= 0 || hours > 8) {
        QMessageBox::warning(this, "Журнал", "Часы пропуска должны быть в диапазоне 1..8.");
        return;
    }

    const QString type = absenceTypeCombo ? absenceTypeCombo->currentData().toString() : "excused";
    if (type != "excused" && type != "unexcused") {
        QMessageBox::warning(this, "Журнал", "Некорректный тип пропуска.");
        return;
    }

    if (!db->upsertAbsenceByKey(studentId, selectedLesson.subjectId, semesterId, hours,
                                selectedLesson.dateISO.toStdString(), type.toStdString())) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить пропуск.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Пропуск сохранён.");
    refreshJournalCurrentValues();
}

void TeacherWindow::onDeleteAbsence()
{
    if (!db) return;
    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите пару.");
        return;
    }
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;
    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Журнал", "Сначала выберите студента.");
        return;
    }
    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const int semesterId = journalSemesterCombo ? journalSemesterCombo->currentData().toInt() : 1;
    if (semesterId <= 0 || studentId <= 0) return;

    int absenceId = 0;
    if (!db->findAbsenceId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), absenceId)) {
        QMessageBox::warning(this, "Журнал", "Ошибка при поиске пропуска.");
        return;
    }
    if (absenceId <= 0) {
        QMessageBox::information(this, "Журнал", "Пропуск не найден.");
        refreshJournalCurrentValues();
        return;
    }

    if (QMessageBox::question(this, "Журнал", "Удалить пропуск?") != QMessageBox::Yes) {
        return;
    }

    if (!db->deleteAbsence(absenceId)) {
        QMessageBox::critical(this, "Журнал", "Не удалось удалить пропуск.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Пропуск удалён.");
    refreshJournalCurrentValues();
}
