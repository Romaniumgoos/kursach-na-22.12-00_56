#include "ui/pages/StudentAbsencesPage.h"

#include "database.h"
#include "ui/util/UiStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizePolicy>

StudentAbsencesPage::StudentAbsencesPage(Database* db, int studentId, QWidget* parent)
    : QWidget(parent)
    , db(db)
    , studentId(studentId)
    , semesterId(1)
    , filterMode(FilterMode::All)
    , semesterCombo(nullptr)
    , filterCombo(nullptr)
    , refreshButton(nullptr)
    , stacked(nullptr)
    , contentWidget(nullptr)
    , emptyWidget(nullptr)
    , table(nullptr)
    , totalAbsencesLabel(nullptr)
    , unexcusedAbsencesLabel(nullptr)
    , emptyStateLabel(nullptr)
    , retryButton(nullptr)
{
    setupLayout();
    setupTable();
    populateSemesters();
    reload();
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

    topBar->addStretch();

    refreshButton = new QPushButton("Обновить", this);
    topBar->addWidget(refreshButton);

    root->addLayout(topBar);

    stacked = new QStackedWidget(this);
    root->addWidget(stacked);

    contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    table = new QTableWidget(contentWidget);
    contentLayout->addWidget(table);

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
        applyFilterToTable();
    });
}

void StudentAbsencesPage::setupTable()
{
    UiStyle::applyStandardTableStyle(table);

    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Дата", "Предмет", "Часы", "Тип"});

    table->setColumnWidth(0, 120);
    table->setColumnWidth(1, 260);
    table->setColumnWidth(2, 70);
    table->setColumnWidth(3, 140);
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

void StudentAbsencesPage::applyFilterToTable()
{
    table->setRowCount(0);

    int shownRows = 0;
    for (const auto& r : cachedAbsences) {
        const bool isExcused = (r.type == "excused");

        if (filterMode == FilterMode::Excused && !isExcused) continue;
        if (filterMode == FilterMode::Unexcused && isExcused) continue;

        const int row = table->rowCount();
        table->insertRow(row);

        const QString date = QString::fromStdString(r.date);
        const QString subject = QString::fromStdString(r.subject);
        const QString typeRu = isExcused ? "Уважительный" : "Неуважительный";

        table->setItem(row, 0, new QTableWidgetItem(date));
        table->setItem(row, 1, new QTableWidgetItem(subject));

        auto* hoursItem = new QTableWidgetItem(QString::number(r.hours));
        hoursItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 2, hoursItem);

        table->setItem(row, 3, new QTableWidgetItem(typeRu));

        QColor rowColor = isExcused ? QColor(220, 245, 220) : QColor(245, 220, 220);
        for (int col = 0; col < 4; ++col) {
            table->item(row, col)->setBackground(rowColor);
        }

        ++shownRows;
    }

    if (shownRows == 0) {
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
    for (const auto& a : absences) {
        AbsenceRow r;
        r.subject = std::get<0>(a);
        r.hours = std::get<1>(a);
        r.date = std::get<2>(a);
        r.type = std::get<3>(a);
        cachedAbsences.push_back(std::move(r));
    }

    updateTotalsFromCache();
    applyFilterToTable();
}
