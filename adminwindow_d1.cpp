#include "adminwindow.h"

#include "services/d1_randomizer.h"
#include "ui/util/AppEvents.h"
#include "ui/util/UiStyle.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

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

}

QWidget* AdminWindow::buildD1RandomizerTab()
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

    controls->addWidget(makeBadge(controlsCard, "Группа", UiStyle::badgeNeutralStyle()), 0, 0);
    d1GroupCombo = new QComboBox(controlsCard);
    d1GroupCombo->setMinimumWidth(140);
    d1GroupCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controls->addWidget(d1GroupCombo, 0, 1);
    reloadGroupsInto(d1GroupCombo, false, true);

    controls->addWidget(makeBadge(controlsCard, "Семестр", UiStyle::badgeNeutralStyle()), 0, 2);
    d1SemesterCombo = new QComboBox(controlsCard);
    d1SemesterCombo->setMinimumWidth(220);
    d1SemesterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controls->addWidget(d1SemesterCombo, 0, 3);

    if (db) {
        std::vector<std::pair<int, std::string>> semesters;
        if (db->getAllSemesters(semesters)) {
            for (const auto& s : semesters) {
                d1SemesterCombo->addItem(QString::fromStdString(s.second), s.first);
            }
        }
    }
    if (d1SemesterCombo && d1SemesterCombo->count() > 0) {
        const int idx = d1SemesterCombo->findData(1);
        if (idx >= 0) d1SemesterCombo->setCurrentIndex(idx);
    }

    d1OverwriteCheck = new QCheckBox("Перезаписать существующие", controlsCard);
    d1OverwriteCheck->setChecked(false);
    controls->addWidget(d1OverwriteCheck, 1, 0, 1, 2);

    d1GenerateButton = new QPushButton("Сгенерировать оценки (D1)", controlsCard);
    controls->addWidget(d1GenerateButton, 1, 2, 1, 2);
    connect(d1GenerateButton, &QPushButton::clicked, this, &AdminWindow::onGenerateD1);

    mainCardLayout->addWidget(controlsCard);

    auto* tableCard = new QFrame(mainCard);
    tableCard->setFrameShape(QFrame::StyledPanel);
    tableCard->setStyleSheet(cardFrameStyle());
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(12, 10, 12, 12);
    tableLayout->setSpacing(10);

    d1TotalsLabel = new QLabel(tableCard);
    d1TotalsLabel->setText("—");
    tableLayout->addWidget(d1TotalsLabel);

    d1StatsTable = new QTableWidget(tableCard);
    UiStyle::applyStandardTableStyle(d1StatsTable);
    d1StatsTable->setColumnCount(2);
    d1StatsTable->setHorizontalHeaderLabels({"Предмет", "Создано"});
    d1StatsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    d1StatsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    d1StatsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d1StatsTable->setSelectionMode(QAbstractItemView::NoSelection);
    d1StatsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(d1StatsTable, 1);

    mainCardLayout->addWidget(tableCard, 1);
    return root;
}

void AdminWindow::onGenerateD1()
{
    if (!db || !d1GroupCombo || !d1SemesterCombo || !d1OverwriteCheck || !d1GenerateButton || !d1StatsTable || !d1TotalsLabel) return;

    const int groupId = d1GroupCombo->currentData().toInt();
    if (groupId <= 0) {
        QMessageBox::warning(this, "D1 Randomizer", "Выберите группу.");
        return;
    }

    const int semesterId = d1SemesterCombo->currentData().toInt();
    if (semesterId <= 0) {
        QMessageBox::warning(this, "D1 Randomizer", "Выберите семестр.");
        return;
    }

    const bool overwrite = d1OverwriteCheck->isChecked();

    const QString prevText = d1GenerateButton->text();
    d1GenerateButton->setEnabled(false);
    d1GenerateButton->setText("Генерация...");

    auto restoreButton = [&]() {
        if (!d1GenerateButton) return;
        d1GenerateButton->setText(prevText);
        d1GenerateButton->setEnabled(true);
    };

    D1Randomizer rnd(*db);
    const auto res = rnd.generateForGroup(groupId, semesterId, overwrite);
    if (!res.ok) {
        restoreButton();
        QMessageBox::critical(this, "D1 Randomizer", QString::fromStdString(res.error));
        return;
    }

    d1StatsTable->setRowCount(0);
    int row = 0;
    for (const auto& it : res.value.createdBySubject) {
        d1StatsTable->insertRow(row);
        d1StatsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(it.first)));
        d1StatsTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.second)));
        row++;
    }

    d1TotalsLabel->setText(QString("Создано: %1 | Обновлено: %2 | Пропущено: %3")
        .arg(res.value.createdTotal)
        .arg(res.value.updatedExistingTotal)
        .arg(res.value.skippedExistingTotal));

    restoreButton();
    AppEvents::instance().emitScheduleChanged();
}
