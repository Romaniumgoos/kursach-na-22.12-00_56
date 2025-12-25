#include "adminwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include "ui/widgets/ThemeToggleWidget.h"
 #include <QToolBar>
 #include <QAction>
 #include "loginwindow.h"

#include "ui/util/UiStyle.h"

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

struct UserEditResult {
    bool accepted = false;
    int id = 0;
    QString name;
    QString username;
    QString password;
    QString role;
    int groupId = 0;
    int subgroup = 0;
};

static bool execUserExistsByUsername(sqlite3* rawDb, const QString& username, int excludeId, bool& outExists)
{
    outExists = false;
    if (!rawDb) return false;
    const char* sql = "SELECT COUNT(*) FROM users WHERE username = ? AND id <> ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outExists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static QString weekdayName(int weekday)
{
    switch (weekday) {
    case 1: return "Пн";
    case 2: return "Вт";
    case 3: return "Ср";
    case 4: return "Чт";
    case 5: return "Пт";
    case 6: return "Сб";
    default: return QString::number(weekday);
    }
}

struct ScheduleEditResult {
    bool accepted = false;
    int id = 0;
    int groupId = 0;
    int subgroup = 0;
    int weekday = 1;
    int lessonNumber = 1;
    int weekOfCycle = 1;
    int subjectId = 0;
    int teacherId = 0;
    QString room;
    QString lessonType;
};

static bool execGetScheduleById(sqlite3* rawDb, int scheduleId, ScheduleEditResult& out)
{
    if (!rawDb || scheduleId <= 0) return false;
    const char* sql = R"SQL(
        SELECT id, groupid, subgroup, weekday, lessonnumber, weekofcycle,
               subjectid, teacherid, COALESCE(room,''), COALESCE(lessontype,'')
        FROM schedule
        WHERE id = ?
    )SQL";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, scheduleId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        out.id = sqlite3_column_int(stmt, 0);
        out.groupId = sqlite3_column_int(stmt, 1);
        out.subgroup = sqlite3_column_int(stmt, 2);
        out.weekday = sqlite3_column_int(stmt, 3);
        out.lessonNumber = sqlite3_column_int(stmt, 4);
        out.weekOfCycle = sqlite3_column_int(stmt, 5);
        out.subjectId = sqlite3_column_int(stmt, 6);
        out.teacherId = sqlite3_column_int(stmt, 7);
        out.room = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        out.lessonType = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

static bool execIsTeacherBusyExcluding(sqlite3* rawDb, int teacherId, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = "SELECT COUNT(*) FROM schedule WHERE teacherid=? AND weekday=? AND lessonnumber=? AND weekofcycle=? AND id<>?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, teacherId);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static bool execIsRoomBusyExcluding(sqlite3* rawDb, const QString& room, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = "SELECT COUNT(*) FROM schedule WHERE room=? AND weekday=? AND lessonnumber=? AND weekofcycle=? AND room IS NOT NULL AND room<>'' AND id<>?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, room.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static bool execIsSlotBusyExcluding(sqlite3* rawDb, int groupId, int subgroup, int weekday, int lessonNumber, int weekOfCycle, int excludeId, bool& outBusy)
{
    outBusy = false;
    if (!rawDb) return false;
    const char* sql = R"SQL(
        SELECT COUNT(*)
        FROM schedule
        WHERE groupid = ?
          AND weekday = ?
          AND lessonnumber = ?
          AND weekofcycle = ?
          AND (subgroup = ? OR subgroup = 0)
          AND id <> ?
    )SQL";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, lessonNumber);
    sqlite3_bind_int(stmt, 4, weekOfCycle);
    sqlite3_bind_int(stmt, 5, subgroup);
    sqlite3_bind_int(stmt, 6, excludeId);
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        outBusy = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW || rc == SQLITE_DONE;
}

static ScheduleEditResult runScheduleEditDialog(QWidget* parent, Database* db, const QString& title, bool isCreate, const ScheduleEditResult& initial)
{
    ScheduleEditResult res = initial;

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setModal(true);

    auto* root = new QVBoxLayout(&dlg);
    auto* card = new QFrame(&dlg);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(cardFrameStyle());
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(10);
    root->addWidget(card);

    auto* form = new QFormLayout();
    form->setSpacing(10);
    cardLayout->addLayout(form);

    auto* groupCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                groupCombo->addItem(QString::fromStdString(g.second), g.first);
            }
        }
    }
    const int gi = groupCombo->findData(initial.groupId);
    if (gi >= 0) groupCombo->setCurrentIndex(gi);
    form->addRow("Группа:", groupCombo);

    auto* weekdayCombo = new QComboBox(&dlg);
    for (int d = 1; d <= 6; ++d) {
        weekdayCombo->addItem(weekdayName(d), d);
    }
    const int wi = weekdayCombo->findData(initial.weekday);
    if (wi >= 0) weekdayCombo->setCurrentIndex(wi);
    form->addRow("День:", weekdayCombo);

    auto* lessonSpin = new QSpinBox(&dlg);
    lessonSpin->setRange(1, 8);
    lessonSpin->setValue(initial.lessonNumber);
    form->addRow("Пара №:", lessonSpin);

    auto* weekCombo = new QComboBox(&dlg);
    weekCombo->addItem("1", 1);
    weekCombo->addItem("2", 2);
    weekCombo->addItem("3", 3);
    weekCombo->addItem("4", 4);
    const int wci = weekCombo->findData(initial.weekOfCycle);
    if (wci >= 0) weekCombo->setCurrentIndex(wci);
    form->addRow("Неделя цикла:", weekCombo);

    auto* subjectCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> subjects;
        if (db->getAllSubjects(subjects)) {
            for (const auto& s : subjects) {
                subjectCombo->addItem(QString::fromStdString(s.second), s.first);
            }
        }
    }
    const int si = subjectCombo->findData(initial.subjectId);
    if (si >= 0) subjectCombo->setCurrentIndex(si);
    form->addRow("Предмет:", subjectCombo);

    auto* teacherCombo = new QComboBox(&dlg);
    if (db) {
        std::vector<std::pair<int, std::string>> teachers;
        if (db->getAllTeachers(teachers)) {
            for (const auto& t : teachers) {
                teacherCombo->addItem(QString::fromStdString(t.second), t.first);
            }
        }
    }
    const int ti = teacherCombo->findData(initial.teacherId);
    if (ti >= 0) teacherCombo->setCurrentIndex(ti);
    form->addRow("Преподаватель:", teacherCombo);

    auto* roomEdit = new QLineEdit(&dlg);
    roomEdit->setText(initial.room);
    form->addRow("Аудитория:", roomEdit);

    auto* typeCombo = new QComboBox(&dlg);
    typeCombo->addItem("ЛК", "ЛК");
    typeCombo->addItem("ПЗ", "ПЗ");
    typeCombo->addItem("ЛР", "ЛР");
    const int lti = typeCombo->findData(initial.lessonType);
    if (lti >= 0) typeCombo->setCurrentIndex(lti);
    form->addRow("Тип:", typeCombo);

    auto* subgroupCombo = new QComboBox(&dlg);
    subgroupCombo->addItem("0", 0);
    subgroupCombo->addItem("1", 1);
    subgroupCombo->addItem("2", 2);
    const int sgi = subgroupCombo->findData(initial.subgroup);
    if (sgi >= 0) subgroupCombo->setCurrentIndex(sgi);
    form->addRow("Подгруппа:", subgroupCombo);

    auto updateLk = [&]() {
        const bool isLk = typeCombo->currentData().toString() == "ЛК";
        subgroupCombo->setEnabled(!isLk);
        if (isLk) {
            const int idx = subgroupCombo->findData(0);
            if (idx >= 0) subgroupCombo->setCurrentIndex(idx);
        }
    };
    updateLk();
    QObject::connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&](int) {
        updateLk();
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        res.accepted = false;
        return res;
    }

    res.groupId = groupCombo->currentData().toInt();
    res.weekday = weekdayCombo->currentData().toInt();
    res.lessonNumber = lessonSpin->value();
    res.weekOfCycle = weekCombo->currentData().toInt();
    res.subjectId = subjectCombo->currentData().toInt();
    res.teacherId = teacherCombo->currentData().toInt();
    res.room = roomEdit->text().trimmed();
    res.lessonType = typeCombo->currentData().toString();
    res.subgroup = subgroupCombo->currentData().toInt();
    if (res.lessonType == "ЛК") {
        res.subgroup = 0;
    }
    res.accepted = true;
    return res;
}

static bool execUpdateUser(sqlite3* rawDb,
                           int userId,
                           const QString& username,
                           const QString& name,
                           const QString& role,
                           int groupId,
                           int subgroup,
                           const QString& passwordOrEmpty)
{
    if (!rawDb || userId <= 0) return false;

    const bool withPassword = !passwordOrEmpty.trimmed().isEmpty();
    const char* sqlNoPass = "UPDATE users SET username=?, name=?, role=?, groupid=?, subgroup=? WHERE id=?;";
    const char* sqlWithPass = "UPDATE users SET username=?, password=?, name=?, role=?, groupid=?, subgroup=? WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(rawDb, withPassword ? sqlWithPass : sqlNoPass, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int idx = 1;
    sqlite3_bind_text(stmt, idx++, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (withPassword) {
        sqlite3_bind_text(stmt, idx++, passwordOrEmpty.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, idx++, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, role.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    if (groupId > 0) sqlite3_bind_int(stmt, idx++, groupId);
    else sqlite3_bind_null(stmt, idx++);
    sqlite3_bind_int(stmt, idx++, subgroup);
    sqlite3_bind_int(stmt, idx++, userId);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

static UserEditResult runUserEditDialog(QWidget* parent,
                                       Database* db,
                                       const QString& title,
                                       bool isCreate,
                                       const UserEditResult& initial)
{
    UserEditResult res = initial;

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setModal(true);

    auto* root = new QVBoxLayout(&dlg);

    auto* card = new QFrame(&dlg);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(cardFrameStyle());
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(10);
    root->addWidget(card);

    auto* form = new QFormLayout();
    form->setSpacing(10);
    cardLayout->addLayout(form);

    auto* nameEdit = new QLineEdit(&dlg);
    nameEdit->setText(initial.name);
    form->addRow("ФИО:", nameEdit);

    auto* loginEdit = new QLineEdit(&dlg);
    loginEdit->setText(initial.username);
    form->addRow("Логин:", loginEdit);

    auto* passEdit = new QLineEdit(&dlg);
    passEdit->setEchoMode(QLineEdit::Password);
    form->addRow("Пароль:", passEdit);
    if (!isCreate) {
        passEdit->setPlaceholderText("(не менять)");
    }

    auto* roleCombo = new QComboBox(&dlg);
    roleCombo->addItem("admin", "admin");
    roleCombo->addItem("teacher", "teacher");
    roleCombo->addItem("student", "student");
    const int roleIdx = roleCombo->findData(initial.role);
    if (roleIdx >= 0) roleCombo->setCurrentIndex(roleIdx);
    form->addRow("Роль:", roleCombo);

    auto* groupCombo = new QComboBox(&dlg);
    groupCombo->addItem("—", 0);
    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                groupCombo->addItem(QString::fromStdString(g.second), g.first);
            }
        }
    }
    const int gIdx = groupCombo->findData(initial.groupId);
    if (gIdx >= 0) groupCombo->setCurrentIndex(gIdx);
    form->addRow("Группа:", groupCombo);

    auto* subgroupCombo = new QComboBox(&dlg);
    subgroupCombo->addItem("0", 0);
    subgroupCombo->addItem("1", 1);
    subgroupCombo->addItem("2", 2);
    const int sgIdx = subgroupCombo->findData(initial.subgroup);
    if (sgIdx >= 0) subgroupCombo->setCurrentIndex(sgIdx);
    form->addRow("Подгруппа:", subgroupCombo);

    auto updateStudentEnabled = [&]() {
        const bool isStudent = roleCombo->currentData().toString() == "student";
        groupCombo->setEnabled(isStudent);
        subgroupCombo->setEnabled(isStudent);
    };
    updateStudentEnabled();
    QObject::connect(roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, [&](int) {
        updateStudentEnabled();
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        res.accepted = false;
        return res;
    }

    res.name = nameEdit->text().trimmed();
    res.username = loginEdit->text().trimmed();
    res.password = passEdit->text();
    res.role = roleCombo->currentData().toString();
    res.groupId = groupCombo->currentData().toInt();
    res.subgroup = subgroupCombo->currentData().toInt();
    res.accepted = true;
    return res;
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
    userSearchEdit->setMinimumWidth(240);
    controls->addWidget(userSearchEdit);

    userRoleFilterCombo = new QComboBox(controlsCard);
    userRoleFilterCombo->setMinimumWidth(160);
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
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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
    auto* controls = new QHBoxLayout(controlsCard);
    controls->setContentsMargins(14, 10, 14, 10);
    controls->setSpacing(10);

    controls->addWidget(makeBadge(controlsCard, "Группа", UiStyle::badgeNeutralStyle()));
    schedGroupCombo = new QComboBox(controlsCard);
    schedGroupCombo->setMinimumWidth(180);
    controls->addWidget(schedGroupCombo);

    controls->addSpacing(12);
    controls->addWidget(makeBadge(controlsCard, "Подгруппа", UiStyle::badgeNeutralStyle()));
    schedSubgroupCombo = new QComboBox(controlsCard);
    schedSubgroupCombo->setMinimumWidth(120);
    schedSubgroupCombo->addItem("Все", 0);
    schedSubgroupCombo->addItem("1", 1);
    schedSubgroupCombo->addItem("2", 2);
    controls->addWidget(schedSubgroupCombo);

    controls->addSpacing(12);
    controls->addWidget(makeBadge(controlsCard, "Неделя", UiStyle::badgeNeutralStyle()));
    schedWeekCombo = new QComboBox(controlsCard);
    schedWeekCombo->setMinimumWidth(120);
    schedWeekCombo->addItem("1", 1);
    schedWeekCombo->addItem("2", 2);
    schedWeekCombo->addItem("3", 3);
    schedWeekCombo->addItem("4", 4);
    controls->addWidget(schedWeekCombo);

    controls->addStretch();

    addScheduleButton = new QPushButton("Добавить", controlsCard);
    editScheduleButton = new QPushButton("Редактировать", controlsCard);
    deleteScheduleButton = new QPushButton("Удалить", controlsCard);
    refreshScheduleButton = new QPushButton("Обновить", controlsCard);
    editScheduleButton->setEnabled(false);
    deleteScheduleButton->setEnabled(false);

    controls->addWidget(addScheduleButton);
    controls->addWidget(editScheduleButton);
    controls->addWidget(deleteScheduleButton);
    controls->addWidget(refreshScheduleButton);

    mainCardLayout->addWidget(controlsCard);

    auto* tableCard = new QFrame(mainCard);
    tableCard->setFrameShape(QFrame::StyledPanel);
    tableCard->setStyleSheet(cardFrameStyle());
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(12, 10, 12, 12);
    tableLayout->setSpacing(10);

    scheduleTable = new QTableWidget(tableCard);
    UiStyle::applyStandardTableStyle(scheduleTable);
    scheduleTable->setColumnCount(8);
    scheduleTable->setHorizontalHeaderLabels({"ID", "День", "№", "Подгруппа", "Предмет", "Преподаватель", "Ауд.", "Тип"});
    scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    scheduleTable->horizontalHeader()->setStretchLastSection(true);
    scheduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    scheduleTable->setSelectionMode(QAbstractItemView::SingleSelection);
    tableLayout->addWidget(scheduleTable, 1);

    mainCardLayout->addWidget(tableCard, 1);

    reloadGroupsInto(schedGroupCombo, false);

    connect(refreshScheduleButton, &QPushButton::clicked, this, &AdminWindow::reloadSchedule);
    connect(addScheduleButton, &QPushButton::clicked, this, &AdminWindow::onAddSchedule);
    connect(editScheduleButton, &QPushButton::clicked, this, &AdminWindow::onEditSchedule);
    connect(deleteScheduleButton, &QPushButton::clicked, this, &AdminWindow::onDeleteSchedule);

    connect(schedGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);
    connect(schedSubgroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);
    connect(schedWeekCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminWindow::reloadSchedule);

    connect(scheduleTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSel = scheduleTable && scheduleTable->selectionModel() && !scheduleTable->selectionModel()->selectedRows().isEmpty();
        if (editScheduleButton) editScheduleButton->setEnabled(hasSel);
        if (deleteScheduleButton) deleteScheduleButton->setEnabled(hasSel);
    });

    reloadSchedule();
    return root;
}

void AdminWindow::reloadUsers()
{
    if (!db || !usersTable) return;
    usersTable->setRowCount(0);

    const QString q = userSearchEdit ? userSearchEdit->text().trimmed() : QString();
    const QString roleFilter = userRoleFilterCombo ? userRoleFilterCombo->currentData().toString().trimmed().toLower() : QString();

    std::vector<std::tuple<int, std::string, std::string, std::string, int, int>> users;
    if (!db->getAllUsers(users)) return;

    for (const auto& u : users) {
        const int id = std::get<0>(u);
        const QString username = QString::fromStdString(std::get<1>(u));
        const QString name = QString::fromStdString(std::get<2>(u));
        const QString role = QString::fromStdString(std::get<3>(u));
        const int groupId = std::get<4>(u);
        const int subgroup = std::get<5>(u);

        if (!roleFilter.isEmpty() && role.trimmed().toLower() != roleFilter) {
            continue;
        }
        if (!q.isEmpty()) {
            const QString hay = (name + " " + username).toLower();
            if (!hay.contains(q.toLower())) {
                continue;
            }
        }

        const int row = usersTable->rowCount();
        usersTable->insertRow(row);
        usersTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
        usersTable->setItem(row, 1, new QTableWidgetItem(name));
        usersTable->setItem(row, 2, new QTableWidgetItem(username));
        usersTable->setCellWidget(row, 3, makeBadge(usersTable, role, roleBadgeStyle(role)));
        usersTable->setItem(row, 4, new QTableWidgetItem(QString::number(groupId)));
        usersTable->setItem(row, 5, new QTableWidgetItem(QString::number(subgroup)));
    }
}

void AdminWindow::reloadGroupsInto(QComboBox* combo, bool withAllOption)
{
    if (!combo) return;
    combo->blockSignals(true);
    const QVariant prev = combo->currentData();
    combo->clear();
    combo->addItem(withAllOption ? "Все" : "Выберите группу", withAllOption ? 0 : -1);

    if (db) {
        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                combo->addItem(QString::fromStdString(g.second), g.first);
            }
        }
    }

    const int idx = combo->findData(prev);
    if (idx >= 0) combo->setCurrentIndex(idx);
    combo->blockSignals(false);
}

void AdminWindow::reloadSchedule()
{
    if (!db || !scheduleTable || !schedGroupCombo || !schedWeekCombo || !schedSubgroupCombo) return;
    scheduleTable->setRowCount(0);

    const int groupId = schedGroupCombo->currentData().toInt();
    const int weekOfCycle = schedWeekCombo->currentData().toInt();
    const int subgroupFilter = schedSubgroupCombo->currentData().toInt();
    if (groupId < 0 || weekOfCycle <= 0) return;

    for (int weekday = 1; weekday <= 6; ++weekday) {
        std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> rows;
        if (!db->getScheduleForGroup(groupId, weekday, weekOfCycle, rows)) {
            continue;
        }

        for (const auto& r : rows) {
            const int id = std::get<0>(r);
            const int lessonNumber = std::get<1>(r);
            const int subgroup = std::get<2>(r);
            const QString subject = QString::fromStdString(std::get<3>(r));
            const QString room = QString::fromStdString(std::get<4>(r));
            const QString lessonType = QString::fromStdString(std::get<5>(r));
            const QString teacherName = QString::fromStdString(std::get<6>(r));

            if (subgroupFilter != 0) {
                if (!(subgroup == 0 || subgroup == subgroupFilter)) {
                    continue;
                }
            }

            const int row = scheduleTable->rowCount();
            scheduleTable->insertRow(row);
            scheduleTable->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
            scheduleTable->setItem(row, 1, new QTableWidgetItem(weekdayName(weekday)));
            scheduleTable->setItem(row, 2, new QTableWidgetItem(QString::number(lessonNumber)));
            const QString sgText = (subgroup == 0) ? QString("Все") : QString::number(subgroup);
            scheduleTable->setCellWidget(row, 3, makeBadge(scheduleTable, sgText, UiStyle::badgeNeutralStyle()));
            scheduleTable->setItem(row, 4, new QTableWidgetItem(subject));
            scheduleTable->setItem(row, 5, new QTableWidgetItem(teacherName));
            scheduleTable->setItem(row, 6, new QTableWidgetItem(room));
            scheduleTable->setCellWidget(row, 7, makeBadge(scheduleTable, lessonType.isEmpty() ? QString("—") : lessonType, UiStyle::badgeLessonTypeStyle(lessonType)));
        }
    }
}

void AdminWindow::onAddUser()
{
    if (!db) return;
    const UserEditResult init;
    UserEditResult res = runUserEditDialog(this, db, "Добавить пользователя", true, init);
    if (!res.accepted) return;

    if (res.name.isEmpty() || res.username.isEmpty() || res.role.isEmpty()) {
        QMessageBox::warning(this, "Пользователи", "Заполните обязательные поля.");
        return;
    }
    if (res.password.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Пользователи", "Пароль обязателен при создании.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Пользователи", "Подгруппа должна быть 0/1/2.");
        return;
    }
    if (res.role == "student" && res.groupId <= 0) {
        QMessageBox::warning(this, "Пользователи", "Для студента выберите группу.");
        return;
    }

    bool exists = false;
    if (!execUserExistsByUsername(db->getHandle(), res.username, 0, exists)) {
        QMessageBox::critical(this, "Пользователи", "Ошибка проверки уникальности логина.");
        return;
    }
    if (exists) {
        QMessageBox::warning(this, "Пользователи", "Такой логин уже существует.");
        return;
    }

    const int groupId = (res.role == "student") ? res.groupId : 0;
    const int subgroup = (res.role == "student") ? res.subgroup : 0;
    if (!db->insertUser(res.username.toStdString(), res.password.toStdString(), res.role.toStdString(), res.name.toStdString(), groupId, subgroup)) {
        QMessageBox::critical(this, "Пользователи", "Не удалось добавить пользователя.");
        return;
    }

    reloadUsers();
}

void AdminWindow::onEditUser()
{
    if (!db || !usersTable || !usersTable->selectionModel()) return;
    const auto sel = usersTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    const int row = sel.front().row();
    const int id = usersTable->item(row, 0)->text().toInt();
    if (id <= 0) return;

    UserEditResult init;
    init.id = id;
    init.name = usersTable->item(row, 1)->text();
    init.username = usersTable->item(row, 2)->text();
    init.role = usersTable->item(row, 3)->text();
    init.groupId = usersTable->item(row, 4)->text().toInt();
    init.subgroup = usersTable->item(row, 5)->text().toInt();

    UserEditResult res = runUserEditDialog(this, db, "Редактировать пользователя", false, init);
    if (!res.accepted) return;

    if (res.name.isEmpty() || res.username.isEmpty() || res.role.isEmpty()) {
        QMessageBox::warning(this, "Пользователи", "Заполните обязательные поля.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Пользователи", "Подгруппа должна быть 0/1/2.");
        return;
    }
    if (res.role == "student" && res.groupId <= 0) {
        QMessageBox::warning(this, "Пользователи", "Для студента выберите группу.");
        return;
    }

    bool exists = false;
    if (!execUserExistsByUsername(db->getHandle(), res.username, id, exists)) {
        QMessageBox::critical(this, "Пользователи", "Ошибка проверки уникальности логина.");
        return;
    }
    if (exists) {
        QMessageBox::warning(this, "Пользователи", "Такой логин уже существует.");
        return;
    }

    const int groupId = (res.role == "student") ? res.groupId : 0;
    const int subgroup = (res.role == "student") ? res.subgroup : 0;
    if (!execUpdateUser(db->getHandle(), id, res.username, res.name, res.role, groupId, subgroup, res.password)) {
        QMessageBox::critical(this, "Пользователи", "Не удалось сохранить изменения.");
        return;
    }

    reloadUsers();
}

void AdminWindow::onDeleteUser()
{
    if (!db || !usersTable || !usersTable->selectionModel()) return;
    const auto sel = usersTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    const int row = sel.front().row();
    const int id = usersTable->item(row, 0)->text().toInt();
    if (id <= 0) return;

    if (QMessageBox::question(this, "Пользователи", "Удалить пользователя?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!db->deleteUserById(id)) {
        QMessageBox::critical(this, "Пользователи", "Не удалось удалить пользователя.");
        return;
    }
    reloadUsers();
}

void AdminWindow::onAddSchedule()
{
    if (!db) return;

    ScheduleEditResult init;
    if (schedGroupCombo) init.groupId = schedGroupCombo->currentData().toInt();
    if (schedSubgroupCombo) init.subgroup = schedSubgroupCombo->currentData().toInt();
    if (schedWeekCombo) init.weekOfCycle = schedWeekCombo->currentData().toInt();

    ScheduleEditResult res = runScheduleEditDialog(this, db, "Добавить запись", true, init);
    if (!res.accepted) return;

    if (res.groupId < 0 || res.subjectId <= 0 || res.teacherId <= 0) {
        QMessageBox::warning(this, "Расписание", "Заполните обязательные поля.");
        return;
    }
    if (res.weekday < 1 || res.weekday > 6 || res.lessonNumber < 1 || res.lessonNumber > 8 || res.weekOfCycle < 1 || res.weekOfCycle > 4) {
        QMessageBox::warning(this, "Расписание", "Некорректные значения дня/пары/недели.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Расписание", "Подгруппа должна быть 0/1/2.");
        return;
    }

    if (db->isTeacherBusy(res.teacherId, res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: преподаватель занят в этот слот.");
        return;
    }
    if (!res.room.isEmpty() && db->isRoomBusy(res.room.toStdString(), res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: аудитория занята в этот слот.");
        return;
    }
    if (db->isScheduleSlotBusy(res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle)) {
        QMessageBox::warning(this, "Расписание", "Конфликт: у группы уже есть занятие в этот слот.");
        return;
    }

    if (!db->addScheduleEntry(res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle,
                             res.subjectId, res.teacherId, res.room.toStdString(), res.lessonType.toStdString())) {
        QMessageBox::critical(this, "Расписание", "Не удалось добавить запись.");
        return;
    }
    reloadSchedule();
}

void AdminWindow::onEditSchedule()
{
    if (!db || !scheduleTable || !scheduleTable->selectionModel()) return;
    const auto sel = scheduleTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    const int row = sel.front().row();
    const int id = scheduleTable->item(row, 0)->text().toInt();
    if (id <= 0) return;

    ScheduleEditResult init;
    if (!execGetScheduleById(db->getHandle(), id, init)) {
        QMessageBox::critical(this, "Расписание", "Не удалось загрузить запись для редактирования.");
        return;
    }

    ScheduleEditResult res = runScheduleEditDialog(this, db, "Редактировать запись", false, init);
    if (!res.accepted) return;

    if (res.groupId < 0 || res.subjectId <= 0 || res.teacherId <= 0) {
        QMessageBox::warning(this, "Расписание", "Заполните обязательные поля.");
        return;
    }
    if (res.weekday < 1 || res.weekday > 6 || res.lessonNumber < 1 || res.lessonNumber > 8 || res.weekOfCycle < 1 || res.weekOfCycle > 4) {
        QMessageBox::warning(this, "Расписание", "Некорректные значения дня/пары/недели.");
        return;
    }
    if (res.subgroup < 0 || res.subgroup > 2) {
        QMessageBox::warning(this, "Расписание", "Подгруппа должна быть 0/1/2.");
        return;
    }

    bool busy = false;
    if (!execIsTeacherBusyExcluding(db->getHandle(), res.teacherId, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
        QMessageBox::critical(this, "Расписание", "Ошибка проверки преподавателя.");
        return;
    }
    if (busy) {
        QMessageBox::warning(this, "Расписание", "Конфликт: преподаватель занят в этот слот.");
        return;
    }

    if (!res.room.isEmpty()) {
        if (!execIsRoomBusyExcluding(db->getHandle(), res.room, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
            QMessageBox::critical(this, "Расписание", "Ошибка проверки аудитории.");
            return;
        }
        if (busy) {
            QMessageBox::warning(this, "Расписание", "Конфликт: аудитория занята в этот слот.");
            return;
        }
    }

    if (!execIsSlotBusyExcluding(db->getHandle(), res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle, id, busy)) {
        QMessageBox::critical(this, "Расписание", "Ошибка проверки слота группы.");
        return;
    }
    if (busy) {
        QMessageBox::warning(this, "Расписание", "Конфликт: у группы уже есть занятие в этот слот.");
        return;
    }

    if (!db->updateScheduleEntry(id, res.groupId, res.subgroup, res.weekday, res.lessonNumber, res.weekOfCycle,
                                res.subjectId, res.teacherId, res.room.toStdString(), res.lessonType.toStdString())) {
        QMessageBox::critical(this, "Расписание", "Не удалось сохранить изменения.");
        return;
    }

    reloadSchedule();
}

void AdminWindow::onDeleteSchedule()
{
    if (!db || !scheduleTable || !scheduleTable->selectionModel()) return;
    const auto sel = scheduleTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    const int row = sel.front().row();
    const int id = scheduleTable->item(row, 0)->text().toInt();
    if (id <= 0) return;

    if (QMessageBox::question(this, "Расписание", "Удалить запись?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!db->deleteScheduleEntry(id)) {
        QMessageBox::critical(this, "Расписание", "Не удалось удалить запись.");
        return;
    }
    reloadSchedule();
}
