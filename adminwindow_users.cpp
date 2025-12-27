#include "adminwindow.h"

#include "ui/util/UiStyle.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <sqlite3.h>

#include <algorithm>

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

    std::vector<int> teacherSubjectIds;
    std::vector<int> teacherGroupIds;
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
    dlg.setMinimumWidth(420);
    dlg.setMaximumWidth(560);

    auto* root = new QVBoxLayout(&dlg);

    auto* card = new QFrame(&dlg);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet("QFrame{border-radius: 14px; border: 1px solid rgba(120,120,120,0.22); background: palette(Base);}"
                        "QLabel{color: palette(Text); background: transparent;}");
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
    roleCombo->view()->setTextElideMode(Qt::ElideRight);
    roleCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    roleCombo->setMinimumContentsLength(10);
    roleCombo->addItem("admin", "admin");
    roleCombo->addItem("teacher", "teacher");
    roleCombo->addItem("student", "student");
    const int roleIdx = roleCombo->findData(initial.role);
    if (roleIdx >= 0) roleCombo->setCurrentIndex(roleIdx);
    form->addRow("Роль:", roleCombo);

    // Teacher assignments
    auto* teacherAssignBox = new QGroupBox("Настройки преподавателя", &dlg);
    auto* teacherAssignLayout = new QVBoxLayout(teacherAssignBox);
    teacherAssignLayout->setContentsMargins(10, 8, 10, 8);
    teacherAssignLayout->setSpacing(8);

    auto* subjectsList = new QListWidget(teacherAssignBox);
    subjectsList->setSelectionMode(QAbstractItemView::MultiSelection);
    subjectsList->setMinimumHeight(140);
    teacherAssignLayout->addWidget(new QLabel("Предметы", teacherAssignBox));
    teacherAssignLayout->addWidget(subjectsList);

    auto* groupsList = new QListWidget(teacherAssignBox);
    groupsList->setSelectionMode(QAbstractItemView::MultiSelection);
    groupsList->setMinimumHeight(120);
    teacherAssignLayout->addWidget(new QLabel("Группы", teacherAssignBox));
    teacherAssignLayout->addWidget(groupsList);

    cardLayout->addWidget(teacherAssignBox);

    // Fill lists
    if (db) {
        std::vector<std::pair<int, std::string>> subjects;
        if (db->getAllSubjects(subjects)) {
            for (const auto& s : subjects) {
                auto* it = new QListWidgetItem(QString::fromStdString(s.second), subjectsList);
                it->setData(Qt::UserRole, s.first);
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(Qt::Unchecked);
            }
        }

        std::vector<std::pair<int, std::string>> groups;
        if (db->getAllGroups(groups)) {
            for (const auto& g : groups) {
                if (g.first < 0) continue;
                auto* it = new QListWidgetItem(QString::fromStdString(g.second), groupsList);
                it->setData(Qt::UserRole, g.first);
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(Qt::Unchecked);
            }
        }
    }

    // Apply initial selections for teacher
    auto applyTeacherSelections = [&]() {
        const int teacherId = initial.id;
        std::vector<int> subjIds;
        std::vector<int> grpIds;
        if (db && teacherId > 0) {
            db->getTeacherSubjectIds(teacherId, subjIds);
            db->getTeacherGroupIds(teacherId, grpIds);
        } else {
            subjIds = initial.teacherSubjectIds;
            grpIds = initial.teacherGroupIds;
        }

        auto markChecked = [](QListWidget* list, const std::vector<int>& ids) {
            for (int i = 0; i < list->count(); ++i) {
                auto* it = list->item(i);
                const int id = it ? it->data(Qt::UserRole).toInt() : 0;
                const bool on = (std::find(ids.begin(), ids.end(), id) != ids.end());
                if (it) it->setCheckState(on ? Qt::Checked : Qt::Unchecked);
            }
        };

        markChecked(subjectsList, subjIds);
        markChecked(groupsList, grpIds);
    };
    applyTeacherSelections();

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
        const bool isTeacher = roleCombo->currentData().toString() == "teacher";
        groupCombo->setEnabled(isStudent);
        subgroupCombo->setEnabled(isStudent);
        teacherAssignBox->setVisible(isTeacher);
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

    res.teacherSubjectIds.clear();
    res.teacherGroupIds.clear();
    if (res.role == "teacher") {
        for (int i = 0; i < subjectsList->count(); ++i) {
            auto* it = subjectsList->item(i);
            if (it && it->checkState() == Qt::Checked) {
                res.teacherSubjectIds.push_back(it->data(Qt::UserRole).toInt());
            }
        }
        for (int i = 0; i < groupsList->count(); ++i) {
            auto* it = groupsList->item(i);
            if (it && it->checkState() == Qt::Checked) {
                res.teacherGroupIds.push_back(it->data(Qt::UserRole).toInt());
            }
        }
    }

    res.accepted = true;
    return res;
}

} // namespace

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
        auto* roleItem = new QTableWidgetItem();
        roleItem->setData(Qt::UserRole, role);
        roleItem->setText("");
        usersTable->setItem(row, 3, roleItem);
        usersTable->setCellWidget(row, 3, makeBadge(usersTable, role, roleBadgeStyle(role)));
        usersTable->setItem(row, 4, new QTableWidgetItem(QString::number(groupId)));
        usersTable->setItem(row, 5, new QTableWidgetItem(QString::number(subgroup)));
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

    if (res.role == "teacher") {
        if (res.teacherSubjectIds.empty()) {
            QMessageBox::warning(this, "Пользователи", "Для преподавателя выберите хотя бы один предмет.");
            return;
        }
        if (res.teacherGroupIds.empty()) {
            QMessageBox::warning(this, "Пользователи", "Для преподавателя выберите хотя бы одну группу.");
            return;
        }
    }

    if (res.role == "student" && res.groupId > 0 && res.subgroup == 0) {
        std::vector<std::pair<int, std::string>> students;
        if (!db->getStudentsOfGroup(res.groupId, students)) {
            QMessageBox::critical(this, "Пользователи", "Не удалось загрузить список студентов группы для авто-подгруппы.");
            return;
        }

        const QString newName = res.name.trimmed();
        int pos = 0;
        for (const auto& s : students) {
            const QString existingName = QString::fromStdString(s.second);
            if (QString::compare(existingName, newName, Qt::CaseInsensitive) > 0) {
                break;
            }
            ++pos;
        }

        const int total = static_cast<int>(students.size()) + 1;
        const int firstHalf = (total + 1) / 2;
        res.subgroup = (pos < firstHalf) ? 1 : 2;
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

    if (res.role == "teacher") {
        int teacherId = 0;
        if (!db->getUserIdByUsername(res.username.toStdString(), teacherId) || teacherId <= 0) {
            QMessageBox::warning(this, "Пользователи", "Пользователь создан, но не удалось получить teacherId для назначения предметов/групп.");
        } else {
            if (!db->setTeacherSubjects(teacherId, res.teacherSubjectIds) || !db->setTeacherGroups(teacherId, res.teacherGroupIds)) {
                QMessageBox::warning(this, "Пользователи", "Пользователь создан, но не удалось сохранить предметы/группы преподавателя.");
            }
        }
    }

    reloadUsers();
}

void AdminWindow::onEditUser()
{
    if (!db || !usersTable || !usersTable->selectionModel()) return;
    const auto sel = usersTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    const int row = sel.front().row();
    const int id = usersTable->item(row, 0) ? usersTable->item(row, 0)->text().toInt() : 0;
    if (id <= 0) return;

    UserEditResult init;
    init.id = id;
    init.name = usersTable->item(row, 1) ? usersTable->item(row, 1)->text() : QString();
    init.username = usersTable->item(row, 2) ? usersTable->item(row, 2)->text() : QString();
    init.role = usersTable->item(row, 3) ? usersTable->item(row, 3)->data(Qt::UserRole).toString() : QString();
    init.groupId = usersTable->item(row, 4) ? usersTable->item(row, 4)->text().toInt() : 0;
    init.subgroup = usersTable->item(row, 5) ? usersTable->item(row, 5)->text().toInt() : 0;

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
    if (!db->updateUser(id,
                        res.username.toStdString(),
                        res.name.toStdString(),
                        res.role.toStdString(),
                        groupId,
                        subgroup,
                        res.password.toStdString())) {
        QMessageBox::critical(this, "Пользователи", "Не удалось сохранить изменения.");
        return;
    }

    // Teacher assignments persistence
    const bool wasTeacher = (init.role.trimmed().toLower() == "teacher");
    const bool isTeacher = (res.role.trimmed().toLower() == "teacher");
    if (isTeacher) {
        if (!db->setTeacherSubjects(id, res.teacherSubjectIds) || !db->setTeacherGroups(id, res.teacherGroupIds)) {
            QMessageBox::warning(this, "Пользователи", "Изменения сохранены, но не удалось обновить предметы/группы преподавателя.");
        }
    } else if (wasTeacher) {
        const std::vector<int> empty;
        db->setTeacherSubjects(id, empty);
        db->setTeacherGroups(id, empty);
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

    const QString role = usersTable->item(row, 3) ? usersTable->item(row, 3)->data(Qt::UserRole).toString().trimmed().toLower() : QString();

    if (role == "teacher") {
        int cnt = 0;
        if (!db->countScheduleEntriesForTeacher(id, cnt)) {
            QMessageBox::critical(this, "Пользователи", "Не удалось посчитать пары преподавателя перед удалением.");
            return;
        }

        const QString text = QString("Удалить преподавателя?\n\nБудут удалены %1 пар(ы) из расписания.").arg(cnt);
        if (QMessageBox::question(this, "Пользователи", text, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        if (!db->deleteTeacherWithDependencies(id)) {
            QMessageBox::critical(this, "Пользователи", "Не удалось удалить преподавателя и его зависимости.");
            return;
        }
    } else {
        if (QMessageBox::question(this, "Пользователи", "Удалить пользователя?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        if (!db->deleteUserById(id)) {
            QMessageBox::critical(this, "Пользователи", "Не удалось удалить пользователя.");
            return;
        }
    }
    reloadUsers();
}
