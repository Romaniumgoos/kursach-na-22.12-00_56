#include "ui/pages/StudentAbsencesPage.h"

#include "database.h"
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
#include <QSet>
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

static QString accentStripeStyleForAbsence(bool excused)
{
    if (excused) return "background: rgba(70, 170, 255, 0.85);";
    return "background: rgba(235, 80, 80, 0.85);";
}

StudentAbsencesPage::StudentAbsencesPage(Database* db, int studentId, QWidget* parent)
    : QWidget(parent)
    , db(db)
    , studentId(studentId)
    , semesterId(1)
    , filterMode(FilterMode::All)
    , semesterCombo(nullptr)
    , filterCombo(nullptr)
    , monthCombo(nullptr)
    , subjectCombo(nullptr)
    , refreshButton(nullptr)
    , stacked(nullptr)
    , contentWidget(nullptr)
    , emptyWidget(nullptr)
    , listScroll(nullptr)
    , listContainer(nullptr)
    , listLayout(nullptr)
    , totalAbsencesLabel(nullptr)
    , unexcusedAbsencesLabel(nullptr)
    , absSubjectMonthLabel(nullptr)
    , absAllMonthLabel(nullptr)
    , emptyStateLabel(nullptr)
    , retryButton(nullptr)
{
    setupLayout();
    populateSemesters();
    reload();
}

int StudentAbsencesPage::subjectIdForCurrentFilter() const
{
    if (!db || !subjectCombo) return 0;
    const QString subjectName = subjectCombo->currentData().toString();
    if (subjectName.trimmed().isEmpty()) return 0;
    int id = 0;
    if (!db->getSubjectIdByName(subjectName.toStdString(), id)) return 0;
    return id;
}

void StudentAbsencesPage::populateSubjectsFromCache()
{
    if (!subjectCombo) return;
    const QString current = subjectCombo->currentText();
    QSignalBlocker b(*subjectCombo);
    subjectCombo->clear();
    subjectCombo->addItem("Все", "");

    QStringList subjects;
    subjects.reserve(static_cast<int>(cachedAbsences.size()));
    for (const auto& r : cachedAbsences) {
        const QString s = QString::fromStdString(r.subject);
        if (!s.isEmpty() && !subjects.contains(s)) subjects.push_back(s);
    }
    subjects.sort();
    for (const auto& s : subjects) {
        subjectCombo->addItem(s, s);
    }

    const int idx = subjectCombo->findText(current);
    if (idx >= 0) subjectCombo->setCurrentIndex(idx);
}

void StudentAbsencesPage::populateMonthsFromCache()
{
    if (!monthCombo) return;

    const int currentYm = (selectedYear > 0 && selectedMonth > 0) ? (selectedYear * 100 + selectedMonth) : 0;
    const QDate today = QDate::currentDate();
    const int todayYm = today.year() * 100 + today.month();

    QSignalBlocker b(*monthCombo);
    monthCombo->clear();

    QSet<int> ymSet;
    for (const auto& r : cachedAbsences) {
        const QDate d = QDate::fromString(QString::fromStdString(r.date), "yyyy-MM-dd");
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

void StudentAbsencesPage::updateMonthlyStats()
{
    if (!absSubjectMonthLabel || !absAllMonthLabel) return;
    absSubjectMonthLabel->setText("Пропуски по выбранному предмету за месяц: —");
    absAllMonthLabel->setText("Пропуски по всем предметам за месяц: —");

    if (!db || semesterId <= 0) return;
    if (selectedYear <= 0 || selectedMonth <= 0) return;

    int allTotal = 0;
    int allUnexc = 0;
    if (db->getStudentAbsenceHoursForMonth(studentId, semesterId, selectedYear, selectedMonth, /*subjectId*/0, allTotal, allUnexc) && (allTotal > 0 || allUnexc > 0)) {
        absAllMonthLabel->setText(QString("Пропуски по всем предметам за месяц: %1 ч (неуваж %2)").arg(allTotal).arg(allUnexc));
    } else {
        absAllMonthLabel->setText("Пропуски по всем предметам за месяц: —");
    }

    const int subjId = subjectIdForCurrentFilter();
    if (subjId <= 0) {
        absSubjectMonthLabel->setText("Пропуски по выбранному предмету за месяц: —");
        return;
    }

    int subTotal = 0;
    int subUnexc = 0;
    if (db->getStudentAbsenceHoursForMonth(studentId, semesterId, selectedYear, selectedMonth, subjId, subTotal, subUnexc) && (subTotal > 0 || subUnexc > 0)) {
        absSubjectMonthLabel->setText(QString("Пропуски по выбранному предмету за месяц: %1 ч (неуваж %2)").arg(subTotal).arg(subUnexc));
    } else {
        absSubjectMonthLabel->setText("Пропуски по выбранному предмету за месяц: —");
    }
}

void StudentAbsencesPage::setupLayout()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);

    topBar->addWidget(new QLabel("Семестр:", this));
    semesterCombo = new QComboBox(this);
    semesterCombo->setMinimumWidth(140);
    topBar->addWidget(semesterCombo);

    topBar->addSpacing(12);
    topBar->addWidget(new QLabel("Фильтр:", this));
    filterCombo = new QComboBox(this);
    filterCombo->addItem("Все", static_cast<int>(FilterMode::All));
    filterCombo->addItem("Уважительные", static_cast<int>(FilterMode::Excused));
    filterCombo->addItem("Неуважительные", static_cast<int>(FilterMode::Unexcused));
    filterCombo->setMinimumWidth(160);
    topBar->addWidget(filterCombo);

    topBar->addSpacing(12);
    topBar->addWidget(new QLabel("Месяц:", this));
    monthCombo = new QComboBox(this);
    monthCombo->setMinimumWidth(180);
    monthCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topBar->addWidget(monthCombo);

    topBar->addSpacing(12);
    topBar->addWidget(new QLabel("Предмет:", this));
    subjectCombo = new QComboBox(this);
    subjectCombo->setMinimumWidth(200);
    subjectCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topBar->addWidget(subjectCombo);

    topBar->addStretch();

    refreshButton = new QPushButton("Обновить", this);
    topBar->addWidget(refreshButton);

    root->addLayout(topBar);

    stacked = new QStackedWidget(this);
    root->addWidget(stacked);

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

    updateMonthlyStats();
    listScroll->setWidget(listContainer);

    contentLayout->addWidget(listScroll);

    auto* totalsRow = new QHBoxLayout();
    totalsRow->setContentsMargins(0, 0, 0, 0);

    totalAbsencesLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(totalAbsencesLabel);
    totalsRow->addWidget(totalAbsencesLabel);

    unexcusedAbsencesLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(unexcusedAbsencesLabel);
    totalsRow->addWidget(unexcusedAbsencesLabel);

    totalsRow->addStretch();
    contentLayout->addLayout(totalsRow);

    auto* monthStatsRow = new QHBoxLayout();
    monthStatsRow->setContentsMargins(0, 0, 0, 0);
    monthStatsRow->setSpacing(10);

    absSubjectMonthLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(absSubjectMonthLabel);
    monthStatsRow->addWidget(absSubjectMonthLabel);

    absAllMonthLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(absAllMonthLabel);
    monthStatsRow->addWidget(absAllMonthLabel);

    monthStatsRow->addStretch();
    contentLayout->addLayout(monthStatsRow);

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

    connect(refreshButton, &QPushButton::clicked, this, &StudentAbsencesPage::reload);
    connect(retryButton, &QPushButton::clicked, this, &StudentAbsencesPage::reload);

    connect(semesterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        semesterId = semesterCombo->currentData().toInt();
        if (semesterId <= 0) semesterId = 1;
        reload();
    });

    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        filterMode = static_cast<FilterMode>(filterCombo->currentData().toInt());
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

    connect(subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        applyFilterToTimeline();
    });
}

QWidget* StudentAbsencesPage::buildAbsenceCard(const AbsenceRow& r)
{
    auto* card = new QFrame(contentWidget);
    card->setStyleSheet(cardFrameStyle());
    card->setFrameShape(QFrame::StyledPanel);

    auto* outer = new QHBoxLayout(card);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    const bool isExcused = (r.type == "excused");
    auto* stripe = new QFrame(card);
    stripe->setFixedWidth(6);
    stripe->setStyleSheet(accentStripeStyleForAbsence(isExcused));
    outer->addWidget(stripe);

    auto* body = new QWidget(card);
    outer->addWidget(body, 1);

    auto* grid = new QGridLayout(body);
    grid->setContentsMargins(14, 12, 14, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);

    const QString date = QString::fromStdString(r.date);
    const QString time = QString::fromStdString(r.lessonTime);
    const QString subject = QString::fromStdString(r.subject);
    const QString lessonType = QString::fromStdString(r.lessonType);

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

    auto* hoursBadge = new QLabel(QString("%1 ч").arg(r.hours), right);
    hoursBadge->setAlignment(Qt::AlignCenter);
    hoursBadge->setMinimumHeight(22);
    hoursBadge->setStyleSheet(UiStyle::badgeNeutralStyle());
    rightCol->addWidget(hoursBadge, 0, Qt::AlignRight);

    auto* typeBadge = new QLabel(isExcused ? "Уважительный" : "Неуважительный", right);
    typeBadge->setAlignment(Qt::AlignCenter);
    typeBadge->setMinimumHeight(22);
    typeBadge->setStyleSheet(UiStyle::badgeAbsenceStyle(isExcused));
    rightCol->addWidget(typeBadge, 0, Qt::AlignRight);

    if (!lessonType.isEmpty() && lessonType != "—") {
        auto* ltBadge = new QLabel(lessonType, right);
        ltBadge->setAlignment(Qt::AlignCenter);
        ltBadge->setMinimumHeight(22);
        ltBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(lessonType));
        rightCol->addWidget(ltBadge, 0, Qt::AlignRight);
    }
    rightCol->addStretch();

    grid->addWidget(right, 0, 2, 2, 1, Qt::AlignRight | Qt::AlignTop);

    return card;
}

void StudentAbsencesPage::populateSemesters()
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

void StudentAbsencesPage::showEmptyState(const QString& message)
{
    if (emptyStateLabel) emptyStateLabel->setText(message);
    if (stacked) stacked->setCurrentWidget(emptyWidget);
}

void StudentAbsencesPage::showContent()
{
    if (stacked) stacked->setCurrentWidget(contentWidget);
}

void StudentAbsencesPage::updateTotalsFromCache()
{
    int totalHours = 0;
    int unexcusedHoursFromCache = 0;
    for (const auto& r : cachedAbsences) {
        totalHours += r.hours;
        if (r.type != "excused") {
            unexcusedHoursFromCache += r.hours;
        }
    }

    int unexcusedHours = unexcusedHoursFromCache;
    if (db) {
        int tmp = 0;
        if (db->getStudentUnexcusedAbsences(studentId, semesterId, tmp)) {
            unexcusedHours = tmp;
        }
    }

    if (totalAbsencesLabel) {
        totalAbsencesLabel->setText(QString("Всего пропущено: %1 часов").arg(totalHours));
    }
    if (unexcusedAbsencesLabel) {
        unexcusedAbsencesLabel->setText(QString("Неуважительных: %1 часов").arg(unexcusedHours));
    }
}

void StudentAbsencesPage::applyFilterToTimeline()
{
    if (!listLayout || !listContainer) return;

    const QString subjectFilter = subjectCombo ? subjectCombo->currentData().toString() : QString();
    const bool monthEnabled = (selectedYear > 0 && selectedMonth > 0);

    while (listLayout->count() > 0) {
        QLayoutItem* it = listLayout->takeAt(0);
        if (it->widget()) {
            it->widget()->deleteLater();
        }
        delete it;
    }

    std::vector<const AbsenceRow*> rows;
    rows.reserve(cachedAbsences.size());
    for (const auto& r : cachedAbsences) {
        const bool isExcused = (r.type == "excused");
        if (filterMode == FilterMode::Excused && !isExcused) continue;
        if (filterMode == FilterMode::Unexcused && isExcused) continue;

        const QString subject = QString::fromStdString(r.subject);
        if (!subjectFilter.isEmpty() && subject != subjectFilter) continue;

        if (monthEnabled) {
            const QDate d = QDate::fromString(QString::fromStdString(r.date), "yyyy-MM-dd");
            if (!d.isValid()) continue;
            if (d.year() != selectedYear || d.month() != selectedMonth) continue;
        }
        rows.push_back(&r);
    }

    std::sort(rows.begin(), rows.end(), [](const AbsenceRow* a, const AbsenceRow* b) {
        return a->date > b->date;
    });

    int shown = 0;
    int lastYM = -1;
    for (const auto* r : rows) {
        const QDate d = QDate::fromString(QString::fromStdString(r->date), "yyyy-MM-dd");
        const int ym = d.isValid() ? (d.year() * 100 + d.month()) : -1;
        if (ym != lastYM) {
            listLayout->addWidget(buildMonthHeader(listContainer, d));
            lastYM = ym;
        }

        listLayout->addWidget(buildAbsenceCard(*r));
        ++shown;
    }
    listLayout->addStretch();

    if (shown == 0) {
        showEmptyState("Нет пропусков по выбранному фильтру");
    } else {
        showContent();
    }
}

void StudentAbsencesPage::reload()
{
    if (!db) {
        cachedAbsences.clear();
        showEmptyState("Не удалось загрузить пропуски");
        return;
    }

    std::vector<std::tuple<std::string, int, std::string, std::string>> absences;
    if (!db->getStudentAbsencesForSemester(studentId, semesterId, absences)) {
        cachedAbsences.clear();
        showEmptyState("Не удалось загрузить пропуски");
        return;
    }

    cachedAbsences.clear();
    cachedAbsences.reserve(absences.size());

    int groupId = 0;
    int subgroup = 0;
    if (db) {
        db->getStudentGroupAndSubgroup(studentId, groupId, subgroup);
    }

    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    for (const auto& a : absences) {
        AbsenceRow r;
        r.subject = std::get<0>(a);
        r.hours = std::get<1>(a);
        r.date = std::get<2>(a);
        r.type = std::get<3>(a);

        r.lessonTime = "—";
        r.lessonType = "—";
        if (db && groupId > 0 && !r.date.empty() && !r.subject.empty()) {
            const QDate d = QDate::fromString(QString::fromStdString(r.date), "yyyy-MM-dd");
            if (d.isValid()) {
                const int weekday = d.dayOfWeek() - 1;
                const int weekId = db->getWeekIdByDate(r.date);
                const int weekOfCycle = (weekId > 0) ? db->getWeekOfCycleByWeekId(weekId) : 0;

                if (weekday >= 0 && weekOfCycle > 0) {
                    std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> sched;
                    if (db->getScheduleForGroup(groupId, weekday, weekOfCycle, sched)) {
                        for (const auto& s : sched) {
                            const int lessonNumber = std::get<1>(s);
                            const std::string& subj = std::get<3>(s);
                            const std::string& ltype = std::get<5>(s);
                            if (subj == r.subject) {
                                if (lessonNumber >= 1 && lessonNumber <= pairTimes.size()) {
                                    r.lessonTime = pairTimes[lessonNumber - 1].toStdString();
                                }
                                r.lessonType = ltype.empty() ? "—" : ltype;
                                break;
                            }
                        }
                    }
                }
            }
        }

        cachedAbsences.push_back(std::move(r));
    }

    populateSubjectsFromCache();
    populateMonthsFromCache();
    updateTotalsFromCache();
    applyFilterToTimeline();
}
