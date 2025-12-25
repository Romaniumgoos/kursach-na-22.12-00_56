#include "ui/pages/StudentGradesPage.h"

#include "database.h"
#include "statistics.h"
#include "ui/util/UiStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizePolicy>

StudentGradesPage::StudentGradesPage(Database* db, int studentId, QWidget* parent)
    : QWidget(parent)
    , db(db)
    , studentId(studentId)
    , semesterId(1)
    , semesterCombo(nullptr)
    , refreshButton(nullptr)
    , stacked(nullptr)
    , contentWidget(nullptr)
    , emptyWidget(nullptr)
    , table(nullptr)
    , averageLabel(nullptr)
    , emptyStateLabel(nullptr)
    , retryButton(nullptr)
{
    setupLayout();
    setupTable();
    populateSemesters();
    reload();
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

    table = new QTableWidget(contentWidget);
    contentLayout->addWidget(table);

    averageLabel = new QLabel(contentWidget);
    UiStyle::makeInfoLabel(averageLabel);
    contentLayout->addWidget(averageLabel);

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
}

void StudentGradesPage::setupTable()
{
    UiStyle::applyStandardTableStyle(table);

    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Предмет", "Оценка", "Дата", "Тип"});

    table->setColumnWidth(0, 260);
    table->setColumnWidth(1, 80);
    table->setColumnWidth(2, 120);
    table->setColumnWidth(3, 140);
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

    table->setRowCount(0);
    for (const auto& grade : grades) {
        const int row = table->rowCount();
        table->insertRow(row);

        QString subject = QString::fromStdString(std::get<0>(grade));
        int value = std::get<1>(grade);
        QString date = QString::fromStdString(std::get<2>(grade));
        QString type = QString::fromStdString(std::get<3>(grade));

        table->setItem(row, 0, new QTableWidgetItem(subject));

        auto* valueItem = new QTableWidgetItem(QString::number(value));
        valueItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 1, valueItem);

        table->setItem(row, 2, new QTableWidgetItem(date));
        table->setItem(row, 3, new QTableWidgetItem(type));
    }

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
