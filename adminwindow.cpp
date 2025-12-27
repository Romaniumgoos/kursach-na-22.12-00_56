#include "adminwindow.h"
#include <QVBoxLayout>
#include "ui/util/AppEvents.h"
#include "ui/util/UiStyle.h"
#include "ui/widgets/ThemeToggleWidget.h"
#include "ui/widgets/PeriodSelectorWidget.h"
#include <QToolBar>
#include <QAction>
#include "loginwindow.h"

#include <QHBoxLayout>
#include <QFont>

#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QFrame>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QAbstractItemView>

#include <QListWidget>
#include <QGroupBox>
#include <QCheckBox>

#include <algorithm>

#include "ui/util/AppEvents.h"
#include "ui/widgets/WeekGridScheduleWidget.h"
#include "services/d1_randomizer.h"

#include <sqlite3.h>

namespace {

static QString cardFrameStyle()
{
    return "QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}" \
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

static QString roleBadgeStyle(const QString& role)
{
    const QString r = role.trimmed().toLower();
    if (r == "admin") return UiStyle::badgeStyle("rgba(239,68,68,0.62)", "rgba(239,68,68,0.18)", 700);
    if (r == "teacher") return UiStyle::badgeStyle("rgba(59,130,246,0.65)", "rgba(59,130,246,0.20)", 700);
    if (r == "student") return UiStyle::badgeStyle("rgba(34,197,94,0.62)", "rgba(34,197,94,0.18)", 700);
    return UiStyle::badgeNeutralStyle();
}

}

AdminWindow::AdminWindow(Database* db, int adminId, const QString& adminName,
                         QWidget *parent)
    : QMainWindow(parent), db(db), adminId(adminId), adminName(adminName) {

    setupUI();

    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    tb->addWidget(new ThemeToggleWidget(this));
    tb->addSeparator();

    auto* logoutAction = new QAction("Выйти в авторизацию", this);
    tb->addAction(logoutAction);
    connect(logoutAction, &QAction::triggered, this, [this]() {
        this->hide();
        auto* login = new LoginWindow(this->db);
        login->setAttribute(Qt::WA_DeleteOnClose);
        login->show();
        this->close();
    });

    setWindowTitle(QString("Администратор: %1").arg(adminName));
    resize(1100, 750);
}

AdminWindow::~AdminWindow() {
}

void AdminWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(adminName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget(centralWidget);
    mainLayout->addWidget(tabWidget, 1);

    tabWidget->addTab(buildUsersTab(), "Пользователи");
    tabWidget->addTab(buildScheduleTab(), "Расписание");
    tabWidget->addTab(buildD1RandomizerTab(), "D1 Randomizer");
}

QWidget* AdminWindow::buildUsersTab()
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
    auto* controls = new QHBoxLayout(controlsCard);
    controls->setContentsMargins(14, 10, 14, 10);
    controls->setSpacing(10);

    userSearchEdit = new QLineEdit(controlsCard);
    userSearchEdit->setPlaceholderText("Поиск (ФИО / логин)");
    userSearchEdit->setMinimumWidth(160);
    userSearchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    controls->addWidget(userSearchEdit);

    userRoleFilterCombo = new QComboBox(controlsCard);
    userRoleFilterCombo->setMinimumWidth(120);
    userRoleFilterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    userRoleFilterCombo->view()->setTextElideMode(Qt::ElideRight);
    userRoleFilterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    userRoleFilterCombo->setMinimumContentsLength(10);
    userRoleFilterCombo->addItem("Все роли", "");
    userRoleFilterCombo->addItem("admin", "admin");
    userRoleFilterCombo->addItem("teacher", "teacher");
    userRoleFilterCombo->addItem("student", "student");
    controls->addWidget(userRoleFilterCombo);

    controls->addStretch();

    addUserButton = new QPushButton("Добавить", controlsCard);
    editUserButton = new QPushButton("Редактировать", controlsCard);
    deleteUserButton = new QPushButton("Удалить", controlsCard);
    refreshUsersButton = new QPushButton("Обновить", controlsCard);
    editUserButton->setEnabled(false);
    deleteUserButton->setEnabled(false);

    controls->addWidget(addUserButton);
    controls->addWidget(editUserButton);
    controls->addWidget(deleteUserButton);
    controls->addWidget(refreshUsersButton);

    mainCardLayout->addWidget(controlsCard);

    auto* tableCard = new QFrame(mainCard);
    tableCard->setFrameShape(QFrame::StyledPanel);
    tableCard->setStyleSheet(cardFrameStyle());
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(12, 10, 12, 12);
    tableLayout->setSpacing(10);

    usersTable = new QTableWidget(tableCard);
    UiStyle::applyStandardTableStyle(usersTable);
    usersTable->setColumnCount(6);
    usersTable->setHorizontalHeaderLabels({"ID", "ФИО", "login", "role", "groupId", "subgroup"});
    usersTable->setWordWrap(false);
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    usersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    usersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    usersTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    usersTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    usersTable->setColumnWidth(0, 64);
    usersTable->setColumnWidth(3, 116);
    usersTable->setColumnWidth(4, 76);
    usersTable->setColumnWidth(5, 86);
    usersTable->horizontalHeader()->setStretchLastSection(true);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    tableLayout->addWidget(usersTable, 1);

    mainCardLayout->addWidget(tableCard, 1);

    connect(refreshUsersButton, &QPushButton::clicked, this, &AdminWindow::reloadUsers);
    connect(addUserButton, &QPushButton::clicked, this, &AdminWindow::onAddUser);
    connect(editUserButton, &QPushButton::clicked, this, &AdminWindow::onEditUser);
    connect(deleteUserButton, &QPushButton::clicked, this, &AdminWindow::onDeleteUser);

    if (userSearchEdit) {
        connect(userSearchEdit, &QLineEdit::textChanged, this, &AdminWindow::reloadUsers);
    }
    if (userRoleFilterCombo) {
        connect(userRoleFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadUsers);
    }

    connect(usersTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSel = usersTable && usersTable->selectionModel() && !usersTable->selectionModel()->selectedRows().isEmpty();
        if (editUserButton) editUserButton->setEnabled(hasSel);
        if (deleteUserButton) deleteUserButton->setEnabled(hasSel);
    });

    connect(usersTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        onEditUser();
    });

    reloadUsers();
    return root;
}

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
    d1TotalsLabel->setText("\u2014");
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
