#include "teacherwindow.h"

#include "ui/models/WeekSelection.h"
#include "ui/util/UiStyle.h"
#include "ui/widgets/PeriodSelectorWidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>

namespace {

static QString journalTimelineCardStyle()
{
    return "QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Window);}"
           "QLabel{color: palette(WindowText); background: transparent;}";
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

static QString lessonTypeBadgeStyle(const QString& lessonType)
{
    return UiStyle::badgeLessonTypeStyle(lessonType);
}

static QString neutralBadgeStyle()
{
    return UiStyle::badgeNeutralStyle();
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

} // namespace

QWidget* TeacherWindow::buildJournalTab()
{
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);

    journalPeriodSelector = new PeriodSelectorWidget(db, root);
    layout->addWidget(journalPeriodSelector);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);

    topRow->addWidget(new QLabel("Группа:", root));
    journalGroupCombo = new QComboBox(root);
    journalGroupCombo->setMinimumWidth(120);
    journalGroupCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    journalGroupCombo->view()->setTextElideMode(Qt::ElideRight);
    journalGroupCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topRow->addWidget(journalGroupCombo);

    topRow->addStretch();
    layout->addLayout(topRow);

    auto* journalRoot = new QSplitter(Qt::Horizontal, root);
    journalRoot->setChildrenCollapsible(false);
    layout->addWidget(journalRoot, 1);

    // Left panel (CRUD)
    auto* journalLeftPanel = new QWidget(journalRoot);
    journalLeftPanel->setMinimumWidth(260);
    journalLeftPanel->setMaximumWidth(320);
    auto* leftLayout = new QVBoxLayout(journalLeftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    auto* gradeBox = new QGroupBox("Оценка", journalLeftPanel);
    auto* gradeLayout = new QGridLayout(gradeBox);
    gradeLayout->setContentsMargins(10, 8, 10, 8);
    gradeLayout->setHorizontalSpacing(8);
    gradeLayout->setVerticalSpacing(6);

    gradeSpin = new QSpinBox(gradeBox);
    gradeSpin->setRange(-1, 10);
    gradeSpin->setSpecialValueText("—");
    gradeSpin->setValue(-1);
    gradeSpin->setMinimumHeight(28);
    gradeSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    gradeSpin->setToolTip("Для удаления поставьте значение — и нажмите Применить");

    currentGradeLabel = new QLabel("Оценка: —", gradeBox);
    UiStyle::makeInfoLabel(currentGradeLabel);
    currentGradeLabel->setMinimumHeight(36);
    currentGradeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    saveGradeButton = new QPushButton("Применить", gradeBox);
    saveGradeButton->setMinimumHeight(28);
    saveGradeButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    saveGradeButton->setToolTip("Применить: сохранить/изменить оценку. Значение — удаляет запись");

    auto* gradeHint = new QLabel("Удаление: поставьте — и нажмите Применить", gradeBox);
    UiStyle::makeInfoLabel(gradeHint);
    gradeHint->setMinimumHeight(18);
    gradeHint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    gradeLayout->addWidget(gradeSpin, 0, 0);
    gradeLayout->addWidget(saveGradeButton, 0, 1);
    gradeLayout->addWidget(currentGradeLabel, 1, 0, 1, 2);
    gradeLayout->addWidget(gradeHint, 2, 0, 1, 2);
    gradeLayout->setColumnStretch(0, 1);
    gradeLayout->setColumnStretch(1, 0);

    auto* absenceBox = new QGroupBox("Пропуск", journalLeftPanel);
    auto* absenceLayout = new QGridLayout(absenceBox);
    absenceLayout->setContentsMargins(10, 8, 10, 8);
    absenceLayout->setHorizontalSpacing(8);
    absenceLayout->setVerticalSpacing(6);

    absenceHoursSpin = new QSpinBox(absenceBox);
    absenceHoursSpin->setRange(0, 8);
    absenceHoursSpin->setSpecialValueText("—");
    absenceHoursSpin->setValue(0);
    absenceHoursSpin->setMinimumHeight(28);
    absenceHoursSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    absenceHoursSpin->setToolTip("Для удаления поставьте — (0 часов) и нажмите Применить");

    absenceTypeCombo = new QComboBox(absenceBox);
    absenceTypeCombo->addItem("Уважительный", "excused");
    absenceTypeCombo->addItem("Неуважительный", "unexcused");
    absenceTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    absenceTypeCombo->setMinimumHeight(28);
    absenceTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    absenceTypeCombo->setToolTip("Тип пропуска. Для удаления поставьте — в часах и нажмите Применить");

    currentAbsenceLabel = new QLabel("Пропуск: —", absenceBox);
    UiStyle::makeInfoLabel(currentAbsenceLabel);
    currentAbsenceLabel->setMinimumHeight(36);
    currentAbsenceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    saveAbsenceButton = new QPushButton("Применить", absenceBox);
    saveAbsenceButton->setMinimumHeight(28);
    saveAbsenceButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    saveAbsenceButton->setToolTip("Применить: сохранить/изменить пропуск. Часы — удаляют запись");

    auto* absenceHint = new QLabel("Удаление: поставьте — в часах и нажмите Применить", absenceBox);
    UiStyle::makeInfoLabel(absenceHint);
    absenceHint->setMinimumHeight(18);
    absenceHint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    absenceLayout->addWidget(absenceHoursSpin, 0, 0);
    absenceLayout->addWidget(saveAbsenceButton, 0, 1);
    absenceLayout->addWidget(absenceTypeCombo, 1, 0, 1, 2);
    absenceLayout->addWidget(currentAbsenceLabel, 2, 0, 1, 2);
    absenceLayout->addWidget(absenceHint, 3, 0, 1, 2);
    absenceLayout->setColumnStretch(0, 1);
    absenceLayout->setColumnStretch(1, 0);

    leftLayout->addWidget(gradeBox);
    leftLayout->addWidget(absenceBox);
    leftLayout->addStretch();

    journalRoot->addWidget(journalLeftPanel);

    // Right panel
    auto* journalRightPanel = new QWidget(journalRoot);
    auto* rightLayout = new QVBoxLayout(journalRightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    // Selected lesson card
    journalLessonCardFrame = new QFrame(journalRightPanel);
    journalLessonCardFrame->setObjectName("selectedLessonCard");
    journalLessonCardFrame->setProperty("card", "journalLesson");
    journalLessonCardFrame->setProperty("selected", false);
    journalLessonCardFrame->setFrameShape(QFrame::StyledPanel);
    journalLessonCardFrame->setStyleSheet("#selectedLessonCard{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Window);}"
                                          "#selectedLessonCard QLabel{background: transparent; color: palette(WindowText);}");
    journalLessonCardFrame->setMinimumHeight(150);
    journalLessonCardFrame->setMaximumHeight(190);

    auto* cardGrid = new QGridLayout(journalLessonCardFrame);
    cardGrid->setContentsMargins(14, 12, 14, 12);
    cardGrid->setHorizontalSpacing(10);
    cardGrid->setVerticalSpacing(6);

    journalLessonCardTitle = new QLabel("—", journalLessonCardFrame);
    journalLessonCardTitle->setStyleSheet("font-weight: 900; font-size: 18px; color: palette(WindowText);");
    cardGrid->addWidget(journalLessonCardTitle, 0, 0, 1, 2);

    journalLessonCardSubTitle = new QLabel("—", journalLessonCardFrame);
    journalLessonCardSubTitle->setStyleSheet(
        "padding: 4px 10px;"
        "border-radius: 10px;"
        "border: 1px solid rgba(59,130,246,0.45);"
        "background: rgba(59,130,246,0.16);"
        "color: palette(WindowText);"
        "font-weight: 650;"
    );
    cardGrid->addWidget(journalLessonCardSubTitle, 1, 0, 1, 2);

    // Right badges
    auto* badgesRight = new QWidget(journalLessonCardFrame);
    auto* br = new QVBoxLayout(badgesRight);
    br->setContentsMargins(0, 0, 0, 0);
    br->setSpacing(6);

    journalLessonCardNumber = new QLabel("—", badgesRight);
    journalLessonCardNumber->setMinimumHeight(22);
    journalLessonCardNumber->setAlignment(Qt::AlignCenter);
    journalLessonCardNumber->setStyleSheet(UiStyle::badgeNeutralStyle());
    br->addWidget(journalLessonCardNumber);

    journalLessonCardRoom = new QLabel("—", badgesRight);
    journalLessonCardRoom->setMinimumHeight(22);
    journalLessonCardRoom->setAlignment(Qt::AlignCenter);
    journalLessonCardRoom->setStyleSheet(UiStyle::badgeNeutralStyle());
    br->addWidget(journalLessonCardRoom);

    journalLessonCardType = new QLabel("—", badgesRight);
    journalLessonCardType->setMinimumHeight(22);
    journalLessonCardType->setAlignment(Qt::AlignCenter);
    journalLessonCardType->setStyleSheet(UiStyle::badgeNeutralStyle());
    br->addWidget(journalLessonCardType);

    br->addStretch();
    cardGrid->addWidget(badgesRight, 0, 2, 2, 1, Qt::AlignTop);

    // Group/subgroup badges row
    journalLessonCardGroup = new QLabel("—", journalLessonCardFrame);
    journalLessonCardGroup->setMinimumHeight(22);
    journalLessonCardGroup->setAlignment(Qt::AlignCenter);
    journalLessonCardGroup->setStyleSheet(UiStyle::badgeNeutralStyle());

    journalLessonCardSubgroup = new QLabel("—", journalLessonCardFrame);
    journalLessonCardSubgroup->setMinimumHeight(22);
    journalLessonCardSubgroup->setAlignment(Qt::AlignCenter);
    journalLessonCardSubgroup->setStyleSheet(UiStyle::badgeNeutralStyle());

    cardGrid->addWidget(journalLessonCardGroup, 2, 0, 1, 1, Qt::AlignLeft);
    cardGrid->addWidget(journalLessonCardSubgroup, 2, 1, 1, 1, Qt::AlignLeft);

    rightLayout->addWidget(journalLessonCardFrame, 0);

    // Tables splitter
    auto* journalTables = new QSplitter(Qt::Horizontal, journalRightPanel);
    journalTables->setChildrenCollapsible(false);
    journalTables->setStretchFactor(0, 3);
    journalTables->setStretchFactor(1, 2);

    journalLessonsTable = new QTableWidget(journalTables);
    UiStyle::applyStandardTableStyle(journalLessonsTable);
    journalLessonsTable->setColumnCount(6);
    journalLessonsTable->setHorizontalHeaderLabels({"Дата", "День", "Время", "№", "Предмет", "Тип"});
    journalLessonsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    journalLessonsTable->horizontalHeader()->setStretchLastSection(true);
    journalLessonsTable->setAlternatingRowColors(true);
    journalLessonsTable->setStyleSheet(
        "QTableWidget{"
        "alternate-background-color: rgba(59,130,246,0.08);"
        "selection-background-color: rgba(59,130,246,0.28);"
        "selection-color: palette(WindowText);"
        "}"
        "QHeaderView::section{"
        "background: rgba(59,130,246,0.18);"
        "color: palette(WindowText);"
        "font-weight: 800;"
        "padding: 6px 10px;"
        "border: none;"
        "border-bottom: 1px solid rgba(120,120,120,0.25);"
        "}"
        "QTableWidget::item{padding: 4px 8px;}"
        "QTableWidget::item:hover{background: rgba(59,130,246,0.14);}"
        "QTableWidget::item:selected{background: rgba(59,130,246,0.25); color: palette(WindowText);}"
    );
    journalLessonsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    journalLessonsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    journalTables->addWidget(journalLessonsTable);

    journalStudentsTable = new QTableWidget(journalTables);
    UiStyle::applyStandardTableStyle(journalStudentsTable);
    journalStudentsTable->setColumnCount(2);
    journalStudentsTable->setHorizontalHeaderLabels({"ID", "Студент"});
    journalStudentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    journalStudentsTable->horizontalHeader()->setStretchLastSection(true);
    journalStudentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    journalStudentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    journalTables->addWidget(journalStudentsTable);

    rightLayout->addWidget(journalTables, 1);

    journalRoot->addWidget(journalRightPanel);
    journalRoot->setStretchFactor(0, 0);
    journalRoot->setStretchFactor(1, 1);

    connect(journalPeriodSelector, &PeriodSelectorWidget::selectionChanged,
            this, &TeacherWindow::onJournalPeriodChanged);
    connect(journalGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeacherWindow::onJournalGroupChanged);

    connect(journalLessonsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TeacherWindow::onJournalLessonSelectionChanged);

    connect(journalStudentsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TeacherWindow::onJournalStudentSelectionChanged);

    connect(saveGradeButton, &QPushButton::clicked, this, &TeacherWindow::onSaveGrade);
    connect(saveAbsenceButton, &QPushButton::clicked, this, &TeacherWindow::onSaveAbsence);

    onJournalPeriodChanged(journalPeriodSelector->currentSelection());

    return root;
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
    const int semesterId = defaultSemesterId();
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
    const int semesterId = defaultSemesterId();
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
    if (!journalLessonsTable || !journalPeriodSelector || !journalGroupCombo || !journalStudentsTable) return;

    journalLessonsTable->setRowCount(0);

    journalLessonRows.clear();
    selectedLesson = {};
    if (journalLessonCardTitle) journalLessonCardTitle->setText("—");
    if (journalLessonCardSubTitle) journalLessonCardSubTitle->setText("—");
    if (journalLessonCardNumber) journalLessonCardNumber->setText("—");
    if (journalLessonCardRoom) journalLessonCardRoom->setText("—");
    if (journalLessonCardType) journalLessonCardType->setText("—");
    if (journalLessonCardGroup) journalLessonCardGroup->setText("—");
    if (journalLessonCardSubgroup) journalLessonCardSubgroup->setText("—");
    if (saveGradeButton) saveGradeButton->setEnabled(false);
    refreshJournalCurrentValues();

    const int groupId = journalGroupCombo->currentData().toInt();
    if (groupId <= 0 || !db) return;

    int weekId = 0;
    int weekOfCycle = 1;
    QString err;
    if (!resolveWeekSelection(journalPeriodSelector->currentSelection(), weekId, weekOfCycle, err)) {
        return;
    }

    // Strict layout: lessons list should not depend on currently selected student
    const int subgroupFilter = 0;

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

    journalLessonsTable->setRowCount(static_cast<int>(journalLessonRows.size()));
    for (int i = 0; i < static_cast<int>(journalLessonRows.size()); ++i) {
        const auto& rr = journalLessonRows[i];
        const QString dateText = rr.lesson.dateISO.isEmpty() ? QString("—") : formatDdMm(rr.lesson.dateISO);
        const QString dayText = rr.weekdayName.isEmpty() ? QString("—") : rr.weekdayName;
        const QString timeText = rr.timeText.isEmpty() ? QString("—") : rr.timeText;
        const QString numText = rr.lesson.lessonNumber > 0 ? QString::number(rr.lesson.lessonNumber) : QString("—");
        const QString subjText = rr.lesson.subjectName.isEmpty() ? QString("—") : rr.lesson.subjectName;
        const QString typeText = rr.lesson.lessonType.isEmpty() ? QString("—") : rr.lesson.lessonType;

        journalLessonsTable->setItem(i, 0, new QTableWidgetItem(dateText));
        journalLessonsTable->setItem(i, 1, new QTableWidgetItem(dayText));
        journalLessonsTable->setItem(i, 2, new QTableWidgetItem(timeText));
        journalLessonsTable->setItem(i, 3, new QTableWidgetItem(numText));
        journalLessonsTable->setItem(i, 4, new QTableWidgetItem(subjText));
        journalLessonsTable->setItem(i, 5, new QTableWidgetItem(typeText));
    }
}

void TeacherWindow::onJournalLessonSelectionChanged()
{
    if (!journalLessonsTable || !journalLessonsTable->selectionModel()) return;
    const auto sel = journalLessonsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        selectedLesson = {};
        if (journalLessonCardFrame) {
            journalLessonCardFrame->setProperty("selected", false);
            journalLessonCardFrame->style()->unpolish(journalLessonCardFrame);
            journalLessonCardFrame->style()->polish(journalLessonCardFrame);
            journalLessonCardFrame->update();
        }
        refreshJournalCurrentValues();
        return;
    }
    const int row = sel.front().row();
    if (row < 0 || row >= static_cast<int>(journalLessonRows.size())) return;
    onJournalLessonCardClicked(row);
}

bool TeacherWindow::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    Q_UNUSED(event);
    return QMainWindow::eventFilter(watched, event);
}

void TeacherWindow::onJournalLessonCardClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(journalLessonRows.size())) return;
    selectedLesson = journalLessonRows[index].lesson;

    if (journalLessonCardFrame) {
        journalLessonCardFrame->setProperty("selected", true);
        journalLessonCardFrame->style()->unpolish(journalLessonCardFrame);
        journalLessonCardFrame->style()->polish(journalLessonCardFrame);
        journalLessonCardFrame->update();
    }

    const QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    const QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    const QString dateText = selectedLesson.dateISO.isEmpty() ? "—" : formatDdMm(selectedLesson.dateISO);
    const QString weekdayText = (selectedLesson.weekday >= 0 && selectedLesson.weekday < dayNames.size()) ? dayNames[selectedLesson.weekday] : "—";
    const QString timeText = (selectedLesson.lessonNumber >= 1 && selectedLesson.lessonNumber <= 6) ? pairTimes[selectedLesson.lessonNumber - 1] : "—";

    const QString title = selectedLesson.subjectName.isEmpty() ? QString("—") : selectedLesson.subjectName;
    const QString subtitle = QString("%1 - %2 - %3").arg(dateText).arg(weekdayText).arg(timeText);

    if (journalLessonCardTitle) journalLessonCardTitle->setText(title);
    if (journalLessonCardSubTitle) journalLessonCardSubTitle->setText(subtitle);

    if (journalLessonCardNumber) {
        journalLessonCardNumber->setText(selectedLesson.lessonNumber > 0 ? QString("№%1").arg(selectedLesson.lessonNumber) : QString("—"));
        journalLessonCardNumber->setStyleSheet(UiStyle::badgeNeutralStyle());
    }
    if (journalLessonCardRoom) {
        journalLessonCardRoom->setText(selectedLesson.room.isEmpty() ? "—" : selectedLesson.room);
        journalLessonCardRoom->setStyleSheet(UiStyle::badgeNeutralStyle());
    }
    if (journalLessonCardType) {
        const QString t = selectedLesson.lessonType.isEmpty() ? "—" : selectedLesson.lessonType;
        journalLessonCardType->setText(t);
        journalLessonCardType->setStyleSheet(selectedLesson.lessonType.isEmpty() ? UiStyle::badgeNeutralStyle() : UiStyle::badgeLessonTypeStyle(selectedLesson.lessonType));
    }

    const QString groupText = journalGroupCombo ? journalGroupCombo->currentText() : QString();
    if (journalLessonCardGroup) {
        journalLessonCardGroup->setText(groupText.isEmpty() ? "—" : groupText);
        journalLessonCardGroup->setStyleSheet(UiStyle::badgeNeutralStyle());
    }
    if (journalLessonCardSubgroup) {
        QString sgText = "—";
        if (selectedLesson.subgroup == 1) sgText = "ПГ1";
        else if (selectedLesson.subgroup == 2) sgText = "ПГ2";
        journalLessonCardSubgroup->setText(sgText);
        journalLessonCardSubgroup->setStyleSheet(UiStyle::badgeNeutralStyle());
    }

    if (saveGradeButton) {
        saveGradeButton->setEnabled(!selectedLesson.lessonType.contains("ЛК"));
    }

    refreshJournalCurrentValues();
}

void TeacherWindow::onJournalStudentSelectionChanged()
{
    refreshJournalCurrentValues();
}

void TeacherWindow::refreshJournalCurrentValues()
{
    if (!db) return;
    if (!currentGradeLabel || !currentAbsenceLabel) return;

    currentGradeLabel->setText("Оценка: —");
    currentAbsenceLabel->setText("Пропуск: —");

    if (!selectedLesson.valid || selectedLesson.subjectId <= 0 || selectedLesson.dateISO.isEmpty()) return;
    if (!journalStudentsTable || !journalStudentsTable->selectionModel()) return;

    const auto sel = journalStudentsTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;

    const int sRow = sel.front().row();
    const int studentId = journalStudentsTable->item(sRow, 0)->text().toInt();
    const int semesterId = defaultSemesterId();
    if (studentId <= 0 || semesterId <= 0) return;

    int gradeId = 0;
    if (db->findGradeId(studentId, selectedLesson.subjectId, semesterId, selectedLesson.dateISO.toStdString(), gradeId) && gradeId > 0) {
        int value = 0;
        std::string date;
        std::string type;
        if (db->getGradeById(gradeId, value, date, type)) {
            currentGradeLabel->setText(QString("Оценка: %1").arg(value));
            if (gradeSpin) gradeSpin->setValue(value);
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

    const int semesterId = defaultSemesterId();
    if (semesterId <= 0) return;

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

    const int gradeValue = gradeSpin ? gradeSpin->value() : -1;
    if (gradeValue == -1) {
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
        if (QMessageBox::question(this, "Журнал", "Удалить оценку?") != QMessageBox::Yes) return;
        if (!db->deleteGrade(gradeId)) {
            QMessageBox::critical(this, "Журнал", "Не удалось удалить оценку.");
            return;
        }
        QMessageBox::information(this, "Журнал", "Оценка удалена.");
        refreshJournalCurrentValues();
        return;
    }

    if (gradeValue < 0 || gradeValue > 10) {
        QMessageBox::warning(this, "Журнал", "Оценка должна быть в диапазоне 0..10 (или — для удаления).");
        return;
    }

    if (!db->upsertGradeByKey(studentId, selectedLesson.subjectId, semesterId, gradeValue,
                              selectedLesson.dateISO.toStdString(), /*gradeType*/"")) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить оценку.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Оценка применена.");
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

    const int semesterId = defaultSemesterId();
    if (semesterId <= 0) return;

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
    if (hours < 0 || hours > 8) {
        QMessageBox::warning(this, "Журнал", "Часы пропуска должны быть в диапазоне 0..8 (или — для удаления).");
        return;
    }

    const QString type = absenceTypeCombo ? absenceTypeCombo->currentData().toString() : "excused";
    if (type != "excused" && type != "unexcused") {
        QMessageBox::warning(this, "Журнал", "Некорректный тип пропуска.");
        return;
    }

    if (hours == 0) {
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
        if (QMessageBox::question(this, "Журнал", "Удалить пропуск?") != QMessageBox::Yes) return;
        if (!db->deleteAbsence(absenceId)) {
            QMessageBox::critical(this, "Журнал", "Не удалось удалить пропуск.");
            return;
        }
        QMessageBox::information(this, "Журнал", "Пропуск удалён.");
        refreshJournalCurrentValues();
        return;
    }

    if (!db->upsertAbsenceByKey(studentId, selectedLesson.subjectId, semesterId, hours,
                                selectedLesson.dateISO.toStdString(), type.toStdString())) {
        QMessageBox::critical(this, "Журнал", "Не удалось сохранить пропуск.");
        return;
    }

    QMessageBox::information(this, "Журнал", "Пропуск применён.");
    refreshJournalCurrentValues();
}
