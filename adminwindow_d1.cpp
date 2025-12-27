#include "adminwindow.h"

#include "services/d1_randomizer.h"
#include "ui/util/AppEvents.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

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
