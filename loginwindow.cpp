#include "loginwindow.h"
#include "studentwindow.h"
#include "teacherwindow.h"
#include "adminwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDebug>

LoginWindow::LoginWindow(Database* db, QWidget *parent)
    : QMainWindow(parent), db_(db) {
    setupUI();
    setWindowTitle("Авторизация - Student Management System");
    resize(400, 250);
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

    usernameEdit_ = new QLineEdit(this);
    usernameEdit_->setPlaceholderText("Введите логин");
    usernameEdit_->setMinimumHeight(35);
    formLayout->addRow("Логин:", usernameEdit_);

    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setPlaceholderText("Введите пароль");
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setMinimumHeight(35);
    formLayout->addRow("Пароль:", passwordEdit_);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);

    // Кнопка входа
    loginButton_ = new QPushButton("Войти", this);
    loginButton_->setMinimumHeight(40);
    loginButton_->setDefault(true);
    mainLayout->addWidget(loginButton_);

    // Статус
    statusLabel_ = new QLabel(this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("color: red;");
    mainLayout->addWidget(statusLabel_);

    mainLayout->addStretch();

    // Подключить сигналы
    connect(loginButton_, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onLoginClicked() {
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        statusLabel_->setText("Введите логин и пароль");
        return;
    }

    // Авторизация через Database
    int userId = 0;
    std::string userName, userRole;

    bool success = db_->findUser(
        username.toStdString(),
        password.toStdString(),
        userId,
        userName,
        userRole
    );

    if (!success) {
        statusLabel_->setText("Неверный логин или пароль");
        passwordEdit_->clear();
        passwordEdit_->setFocus();
        return;
    }

    // Успешная авторизация - открыть нужное окно
    statusLabel_->setText("");
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
    StudentWindow* studentWin = new StudentWindow(db_, userId, userName);
    studentWin->show();
    this->hide();

    // Закрыть окно логина при закрытии окна студента
    connect(studentWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}

void LoginWindow::openTeacherWindow(int userId, const QString& userName) {
    TeacherWindow* teacherWin = new TeacherWindow(db_, userId, userName);
    teacherWin->show();
    this->hide();

    connect(teacherWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}

void LoginWindow::openAdminWindow(int userId, const QString& userName) {
    AdminWindow* adminWin = new AdminWindow(db_, userId, userName);
    adminWin->show();
    this->hide();

    connect(adminWin, &QMainWindow::destroyed, this, &QMainWindow::close);
}
