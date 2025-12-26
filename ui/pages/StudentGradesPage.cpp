#include "ui/pages/StudentGradesPage.h"

#include "database.h"
#include "statistics.h"
#include "ui/util/UiStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QDate>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QSignalBlocker>
#include <algorithm>

static QString cardFrameStyle()
{
    return "QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Window);}"
           "QLabel{color: palette(WindowText); background: transparent;}";
}

static QWidget* buildMonthHeader(QWidget* parent, const QDate& d)
{
    auto* w = new QWidget(parent);
    auto* row = new QHBoxLayout(w);
    row->setContentsMargins(6, 8, 6, 6);
    row->setSpacing(10);

    const QString title = d.isValid() ? d.toString("MMMM yyyy") : QString("—");

    auto* label = new QLabel(title, w);
    label->setStyleSheet("font-weight: 900; font-size: 13px; color: palette(WindowText);");
    row->addWidget(label);

    auto* line = new QFrame(w);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: rgba(120,120,120,0.35);");
    row->addWidget(line, 1);

    return w;
}

static QString accentStripeStyleForLessonType(const QString& lessonType)
{
    if (lessonType.contains("ЛР", Qt::CaseInsensitive)) return "background: rgba(255, 145, 80, 0.85);";
    if (lessonType.contains("ПЗ", Qt::CaseInsensitive)) return "background: rgba(70, 170, 255, 0.85);";
    if (lessonType.contains("ЛК", Qt::CaseInsensitive)) return "background: rgba(170, 120, 255, 0.85);";
    return "background: rgba(120,120,120,0.55);";
}

StudentGradesPage::StudentGradesPage(Database* db, int studentId, QWidget* parent)
    : QWidget(parent)
    , db(db)
    , studentId(studentId)
    , semesterId(1)
    , semesterCombo(nullptr)
    , subjectCombo(nullptr)
    , monthCombo(nullptr)
    , refreshButton(nullptr)
    , stacked(nullptr)
    , contentWidget(nullptr)
    , emptyWidget(nullptr)
    , listScroll(nullptr)
    , listContainer(nullptr)
    , listLayout(nullptr)
    , averageLabel(nullptr)
    , countLabel(nullptr)
    , avgSubjectMonthLabel(nullptr)
    , avgAllMonthLabel(nullptr)
    , emptyStateLabel(nullptr)
    , retryButton(nullptr)
{
    setupLayout();
    populateSemesters();
    reload();
}

int StudentGradesPage::subjectIdForCurrentFilter() const
{
    if (!db || !subjectCombo) return 0;
    const QString subjectName = subjectCombo->currentData().toString();
    if (subjectName.trimmed().isEmpty()) return 0;

    int id = 0;
    if (!db->getSubjectIdByName(subjectName.toStdString(), id)) return 0;
    return id;
}

void StudentGradesPage::populateSubjectsFromCache()
{
    if (!subjectCombo) return;

    const QString current = subjectCombo->currentText();
    subjectCombo->blockSignals(true);
    subjectCombo->clear();
    subjectCombo->addItem("Все", "");

    QStringList subjects;
    subjects.reserve(static_cast<int>(cachedGrades.size()));
    for (const auto& g : cachedGrades) {
        const QString s = QString::fromStdString(g.subject);
        if (!s.isEmpty() && !subjects.contains(s)) subjects.push_back(s);
    }
    subjects.sort();

    for (const auto& s : subjects) {
        subjectCombo->addItem(s, s);
    }

    const int idx = subjectCombo->findText(current);
    if (idx >= 0) subjectCombo->setCurrentIndex(idx);
    subjectCombo->blockSignals(false);
}

void StudentGradesPage::populateMonthsFromCache()
{
    if (!monthCombo) return;

    const int currentYm = (selectedYear > 0 && selectedMonth > 0) ? (selectedYear * 100 + selectedMonth) : 0;
    const QDate today = QDate::currentDate();
    const int todayYm = today.year() * 100 + today.month();

    QSignalBlocker b(*monthCombo);
    monthCombo->clear();

    // collect months present in cache
    QSet<int> ymSet;
    for (const auto& g : cachedGrades) {
        const QDate d = QDate::fromString(QString::fromStdString(g.date), "yyyy-MM-dd");
        if (!d.isValid()) continue;
        ymSet.insert(d.year() * 100 + d.month());
    }

    QList<int> yms = ymSet.values();
    std::sort(yms.begin(), yms.end());

    if (yms.isEmpty()) {
        monthCombo->addItem("Месяц: —", 0);
        selectedYear = 0;
        selectedMonth = 0;
        return;
    }

    monthCombo->addItem("Все месяцы", 0);
    for (int ym : yms) {
        const int y = ym / 100;
        const int m = ym % 100;
        const QDate d(y, m, 1);
        const QString label = d.isValid() ? d.toString("MMMM yyyy") : QString("%1-%2").arg(y).arg(m);
        monthCombo->addItem(label, ym);
    }

    int targetYm = currentYm;
    if (targetYm == 0 && ymSet.contains(todayYm)) targetYm = todayYm;
    const int idx = monthCombo->findData(targetYm);
    monthCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    const int sel = monthCombo->currentData().toInt();
    if (sel > 0) {
        selectedYear = sel / 100;
        selectedMonth = sel % 100;
    } else {
        selectedYear = 0;
        selectedMonth = 0;
    }
}

void StudentGradesPage::updateMonthlyStats()
{
    if (!avgSubjectMonthLabel || !avgAllMonthLabel) return;
    avgSubjectMonthLabel->setText("Средний по выбранному предмету: —");
    avgAllMonthLabel->setText("Средний по всем предметам: —");

    if (!db || semesterId <= 0) return;
    if (selectedYear <= 0 || selectedMonth <= 0) return;

    // all subjects
    double allAvg = 0.0;
    int allCnt = 0;
    if (db->getStudentAverageGradeForMonth(studentId, semesterId, selectedYear, selectedMonth, /*subjectId*/0, allAvg, allCnt) && allCnt > 0) {
        avgAllMonthLabel->setText(QString("Средний по всем предметам: %1 (%2)").arg(allAvg, 0, 'f', 2).arg(allCnt));
    } else {
        avgAllMonthLabel->setText("Средний по всем предметам: —");
    }

    const int subjId = subjectIdForCurrentFilter();
    if (subjId <= 0) {
        avgSubjectMonthLabel->setText("Средний по выбранному предмету: —");
        return;
    }
    double subAvg = 0.0;
    int subCnt = 0;
    if (db->getStudentAverageGradeForMonth(studentId, semesterId, selectedYear, selectedMonth, subjId, subAvg, subCnt) && subCnt > 0) {
        avgSubjectMonthLabel->setText(QString("Средний по выбранному предмету: %1 (%2)").arg(subAvg, 0, 'f', 2).arg(subCnt));
    } else {
        avgSubjectMonthLabel->setText("Средний по выбранному предмету: —");
    }
}

QWidget* StudentGradesPage::buildGradeCard(const GradeRow& g)
{
    auto* card = new QFrame(contentWidget);
    card->setStyleSheet(cardFrameStyle());
    card->setFrameShape(QFrame::StyledPanel);

    auto* outer = new QHBoxLayout(card);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    const QString lessonType = QString::fromStdString(g.lessonType);
    auto* stripe = new QFrame(card);
    stripe->setFixedWidth(6);
    stripe->setStyleSheet(accentStripeStyleForLessonType(lessonType));
    outer->addWidget(stripe);

    auto* body = new QWidget(card);
    outer->addWidget(body, 1);

    auto* grid = new QGridLayout(body);
    grid->setContentsMargins(14, 12, 14, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);

    const QString date = QString::fromStdString(g.date);
    const QString time = QString::fromStdString(g.lessonTime);
    const QString subject = QString::fromStdString(g.subject);
    const QString gradeType = QString::fromStdString(g.type);

    auto* title = new QLabel(subject, card);
    title->setStyleSheet("font-weight: 800; font-size: 14px; color: palette(WindowText);");
    grid->addWidget(title, 0, 0, 1, 2);

    auto* meta = new QLabel(QString("%1  •  %2").arg(date, time.isEmpty() ? "—" : time), card);
    meta->setStyleSheet("color: palette(mid); font-weight: 600;");
    grid->addWidget(meta, 1, 0, 1, 2);

    auto* right = new QWidget(card);
    auto* rightCol = new QVBoxLayout(right);
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(6);

    auto* gradeBadge = new QLabel(QString::number(g.value), right);
    gradeBadge->setAlignment(Qt::AlignCenter);
    gradeBadge->setMinimumSize(QSize(34, 34));
    gradeBadge->setStyleSheet(UiStyle::badgeGradeStyle(g.value));
    rightCol->addWidget(gradeBadge, 0, Qt::AlignRight);

    if (!lessonType.isEmpty() && lessonType != "—") {
        auto* ltBadge = new QLabel(lessonType, right);
        ltBadge->setAlignment(Qt::AlignCenter);
        ltBadge->setMinimumHeight(22);
        ltBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(lessonType));
        rightCol->addWidget(ltBadge, 0, Qt::AlignRight);
    }
    rightCol->addStretch();

    grid->addWidget(right, 0, 2, 2, 1, Qt::AlignRight | Qt::AlignTop);

    if (!gradeType.trimmed().isEmpty()) {
        auto* gt = new QLabel(gradeType, card);
        gt->setStyleSheet("color: palette(mid); font-weight: 600;");
        grid->addWidget(gt, 2, 0, 1, 3, Qt::AlignLeft);
    }

    return card;
}

void StudentGradesPage::setupLayout()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    // Верхняя панель
    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);

    auto* semesterLabel = new QLabel("Семестр:", this);
    topBar->addWidget(semesterLabel);

    semesterCombo = new QComboBox(this);
    semesterCombo->setMinimumWidth(140);
    topBar->addWidget(semesterCombo);

    topBar->addSpacing(12);

    auto* subjectLabel = new QLabel("Предмет:", this);
    topBar->addWidget(subjectLabel);

    subjectCombo = new QComboBox(this);
    subjectCombo->setMinimumWidth(200);
    topBar->addWidget(subjectCombo);

    topBar->addSpacing(12);
    auto* monthLabel = new QLabel("Месяц:", this);
    topBar->addWidget(monthLabel);
    monthCombo = new QComboBox(this);
    monthCombo->setMinimumWidth(180);
    monthCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topBar->addWidget(monthCombo);

    topBar->addStretch();

    refreshButton = new QPushButton("Обновить", this);
    topBar->addWidget(refreshButton);

    root->addLayout(topBar);

    stacked = new QStackedWidget(this);
    root->addWidget(stacked);

    // Контент
    contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    listScroll = new QScrollArea(contentWidget);
    listScroll->setWidgetResizable(true);
    listScroll->setFrameShape(QFrame::NoFrame);

    listContainer = new QWidget(listScroll);
    listLayout = new QVBoxLayout(listContainer);
    listLayout->setContentsMargins(10, 10, 10, 10);
    listLayout->setSpacing(10);
    listLayout->addStretch();
    listScroll->setWidget(listContainer);

    contentLayout->addWidget(listScroll);

    auto* summaryRow = new QHBoxLayout();
    summaryRow->setContentsMargins(0, 0, 0, 0);

    averageLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(averageLabel);
    summaryRow->addWidget(averageLabel);

    countLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(countLabel);
    summaryRow->addWidget(countLabel);

    summaryRow->addStretch();
    contentLayout->addLayout(summaryRow);

    auto* monthStatsRow = new QHBoxLayout();
    monthStatsRow->setContentsMargins(0, 0, 0, 0);
    monthStatsRow->setSpacing(10);

    avgSubjectMonthLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(avgSubjectMonthLabel);
    monthStatsRow->addWidget(avgSubjectMonthLabel);

    avgAllMonthLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(avgAllMonthLabel);
    monthStatsRow->addWidget(avgAllMonthLabel);

    monthStatsRow->addStretch();
    contentLayout->addLayout(monthStatsRow);

    // Empty state
    emptyWidget = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    emptyLayout->addStretch();

    emptyStateLabel = new QLabel(emptyWidget);
    emptyStateLabel->setAlignment(Qt::AlignCenter);
    emptyStateLabel->setStyleSheet("color: palette(mid); font-size: 14px;");
    emptyLayout->addWidget(emptyStateLabel);

    retryButton = new QPushButton("Повторить", emptyWidget);
    retryButton->setMaximumWidth(160);
    retryButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    emptyLayout->addWidget(retryButton, 0, Qt::AlignHCenter);
    emptyLayout->addStretch();

    stacked->addWidget(contentWidget);
    stacked->addWidget(emptyWidget);

    connect(refreshButton, &QPushButton::clicked, this, &StudentGradesPage::reload);
    connect(retryButton, &QPushButton::clicked, this, &StudentGradesPage::reload);
    connect(semesterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        semesterId = semesterCombo->currentData().toInt();
        if (semesterId <= 0) semesterId = 1;
        reload();
    });

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        applyFilterToTimeline();
    });

    connect(monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        const int ym = monthCombo ? monthCombo->currentData().toInt() : 0;
        if (ym > 0) {
            selectedYear = ym / 100;
            selectedMonth = ym % 100;
        } else {
            selectedYear = 0;
            selectedMonth = 0;
        }
        applyFilterToTimeline();
    });
}

void StudentGradesPage::applyFilterToTimeline()
{
    if (!listLayout || !listContainer) return;

    const QString filter = subjectCombo ? subjectCombo->currentData().toString() : QString();
    const bool monthEnabled = (selectedYear > 0 && selectedMonth > 0);

    while (listLayout->count() > 0) {
        QLayoutItem* it = listLayout->takeAt(0);
        if (it->widget()) {
            it->widget()->deleteLater();
        }
        delete it;
    }

    std::vector<const GradeRow*> rows;
    rows.reserve(cachedGrades.size());
    for (const auto& g : cachedGrades) {
        const QString subject = QString::fromStdString(g.subject);
        if (!filter.isEmpty() && subject != filter) continue;

        if (monthEnabled) {
            const QDate d = QDate::fromString(QString::fromStdString(g.date), "yyyy-MM-dd");
            if (!d.isValid()) continue;
            if (d.year() != selectedYear || d.month() != selectedMonth) continue;
        }
        rows.push_back(&g);
    }

    std::sort(rows.begin(), rows.end(), [](const GradeRow* a, const GradeRow* b) {
        return a->date > b->date;
    });

    int shown = 0;
    int lastYM = -1;
    for (const auto* g : rows) {
        const QDate d = QDate::fromString(QString::fromStdString(g->date), "yyyy-MM-dd");
        const int ym = d.isValid() ? (d.year() * 100 + d.month()) : -1;
        if (ym != lastYM) {
            listLayout->addWidget(buildMonthHeader(listContainer, d));
            lastYM = ym;
        }

        listLayout->addWidget(buildGradeCard(*g));
        ++shown;
    }
    listLayout->addStretch();

    if (countLabel) {
        countLabel->setText(QString("Оценок: %1").arg(shown));
    }

    updateMonthlyStats();

    if (shown == 0) {
        showEmptyState("Нет оценок по выбранному фильтру");
    } else {
        showContent();
    }
}

void StudentGradesPage::populateSemesters()
{
    semesterCombo->clear();

    std::vector<std::pair<int, std::string>> semesters;
    if (!db || !db->getAllSemesters(semesters) || semesters.empty()) {
        semesterCombo->addItem("Семестр 1", 1);
        semesterId = 1;
        return;
    }

    for (const auto& s : semesters) {
        const int id = s.first;
        const QString name = QString::fromStdString(s.second);
        semesterCombo->addItem(name.isEmpty() ? QString("Семестр %1").arg(id) : name, id);
    }

    semesterId = semesterCombo->currentData().toInt();
    if (semesterId <= 0) semesterId = 1;
}

void StudentGradesPage::showEmptyState(const QString& message)
{
    if (emptyStateLabel) {
        emptyStateLabel->setText(message);
    }
    if (stacked) {
        stacked->setCurrentWidget(emptyWidget);
    }
}

void StudentGradesPage::showContent()
{
    if (stacked) {
        stacked->setCurrentWidget(contentWidget);
    }
}

void StudentGradesPage::reload()
{
    if (!db) {
        showEmptyState("Не удалось загрузить оценки");
        return;
    }

    std::vector<std::tuple<std::string, int, std::string, std::string>> grades;
    if (!db->getStudentGradesForSemester(studentId, semesterId, grades)) {
        showEmptyState("Не удалось загрузить оценки");
        return;
    }

    cachedGrades.clear();
    cachedGrades.reserve(grades.size());

    int groupId = 0;
    int subgroup = 0;
    if (db) {
        db->getStudentGroupAndSubgroup(studentId, groupId, subgroup);
    }

    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };
    for (const auto& grade : grades) {
        GradeRow r;
        r.subject = std::get<0>(grade);
        r.value = std::get<1>(grade);
        r.date = std::get<2>(grade);
        r.type = std::get<3>(grade);

        r.lessonTime = "—";
        r.lessonType = "—";
        if (db && groupId > 0 && !r.date.empty() && !r.subject.empty()) {
            const QDate d = QDate::fromString(QString::fromStdString(r.date), "yyyy-MM-dd");
            if (d.isValid()) {
                const int weekday = d.dayOfWeek();
                const int weekId = db->getWeekIdByDate(r.date);
                const int weekOfCycle = (weekId > 0) ? db->getWeekOfCycleByWeekId(weekId) : 0;

                if (weekday >= 1 && weekday <= 6 && weekOfCycle > 0) {
                    std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> sched;
                    if (db->getScheduleForGroup(groupId, weekday, weekOfCycle, sched)) {
                        int bestLessonNumber = 0;
                        std::string bestLessonType;
                        bool foundAny = false;
                        bool foundPreferred = false;

                        for (const auto& s : sched) {
                            const int lessonNumber = std::get<1>(s);
                            const int schSubgroup = std::get<2>(s);
                            const std::string& subj = std::get<3>(s);
                            const std::string& ltype = std::get<5>(s);
                            if (subj != r.subject) continue;

                            if (!(schSubgroup == 0 || subgroup == 0 || schSubgroup == subgroup)) continue;

                            if (!foundAny) {
                                bestLessonNumber = lessonNumber;
                                bestLessonType = ltype;
                                foundAny = true;
                            }

                            const QString lt = QString::fromStdString(ltype);
                            const bool isLecture = lt.contains("ЛК", Qt::CaseInsensitive);
                            if (!isLecture) {
                                bestLessonNumber = lessonNumber;
                                bestLessonType = ltype;
                                foundPreferred = true;
                                break;
                            }
                        }

                        if (foundAny) {
                            if (bestLessonNumber >= 1 && bestLessonNumber <= pairTimes.size()) {
                                r.lessonTime = pairTimes[bestLessonNumber - 1].toStdString();
                            }
                            (void)foundPreferred;
                            r.lessonType = bestLessonType.empty() ? "—" : bestLessonType;
                        }
                    }
                }
            }
        }
        cachedGrades.push_back(std::move(r));
    }

    populateSubjectsFromCache();
    populateMonthsFromCache();
    applyFilterToTimeline();

    const double average = Statistics::calculateStudentAverage(*db, studentId, semesterId);
    averageLabel->setText(QString("Средний балл: %1").arg(average, 0, 'f', 2));

    QString color = "#d32f2f";
    if (average >= 9.0) color = "#2e7d32";
    else if (average >= 7.0) color = "#1565c0";
    else if (average >= 5.0) color = "#ef6c00";

    QString style = QString(UiStyle::kInfoLabelStyle);
    if (style.endsWith('}')) {
        style.chop(1);
    }
    style += QString("  color: %1; }").arg(color);
    averageLabel->setStyleSheet(style);

    showContent();
}
