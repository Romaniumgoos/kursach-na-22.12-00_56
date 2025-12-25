#include "loginwindow.h"
#include "studentwindow.h"
#include "teacherwindow.h"
#include "adminwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDebug>
#include "ui/widgets/ThemeToggleWidget.h"

LoginWindow::LoginWindow(Database* db, QWidget *parent)
    : QMainWindow(parent), db(db) {
    setupUI();
    setWindowTitle("Авторизация - Student Management System");
    resize(400, 250);

    setMinimumSize(520, 380);
    resize(560, 380);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

}

LoginWindow::~LoginWindow() {
}

void LoginWindow::setupUI() {
    // Центральный виджет
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Главный layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    auto* topRow = new QHBoxLayout();
    topRow->addStretch();
    topRow->addWidget(new ThemeToggleWidget(this));
    mainLayout->addLayout(topRow);

    // Заголовок
    QLabel* titleLabel = new QLabel("Вход в систему", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(20);

    // Форма ввода
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Введите логин");
    usernameEdit->setMinimumHeight(35);
    formLayout->addRow("Логин:", usernameEdit);

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Введите пароль");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(35);
    formLayout->addRow("Пароль:", passwordEdit);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);

    // Кнопка входа
    loginButton = new QPushButton("Войти", this);
    loginButton->setMinimumHeight(40);
    loginButton->setDefault(true);
    mainLayout->addWidget(loginButton);

    // Статус
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(statusLabel);

    mainLayout->addStretch();

    // Подключить сигналы
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onLoginClicked() {
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        statusLabel->setText("Введите логин и пароль");
        return;
    }

    // Авторизация через Database
    int userId = 0;
    std::string userName, userRole;

    bool success = db->findUser(
        username.toStdString(),
        password.toStdString(),
        userId,
        userName,
        userRole
    );

    if (!success) {
        statusLabel->setText("Неверный логин или пароль");
        passwordEdit->clear();
        passwordEdit->setFocus();
        return;
    }

    // Успешная авторизация - открыть нужное окно
    statusLabel->setText("");
    QString userNameQt = QString::fromStdString(userName);

    if (userRole == "student") {
        openStudentWindow(userId, userNameQt);
    } else if (userRole == "teacher") {
        openTeacherWindow(userId, userNameQt);
    } else if (userRole == "admin") {
        openAdminWindow(userId, userNameQt);
    }
}

void LoginWindow::openStudentWindow(int userId, const QString& userName) {
    StudentWindow* studentWin = new StudentWindow(db, userId, userName);
    studentWin->show();
    this->hide();

    // Закрыть окно логина при закрытии окна студента
    connect(studentWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}

void LoginWindow::openTeacherWindow(int userId, const QString& userName) {
    TeacherWindow* teacherWin = new TeacherWindow(db, userId, userName);
    teacherWin->show();
    this->hide();

    connect(teacherWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}

void LoginWindow::openAdminWindow(int userId, const QString& userName) {
    AdminWindow* adminWin = new AdminWindow(db, userId, userName);
    adminWin->show();
    this->hide();

    connect(adminWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}
