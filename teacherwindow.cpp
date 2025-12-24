#include "teacherwindow.h"
#include <QVBoxLayout>
#include <QFont>

TeacherWindow::TeacherWindow(Database* db, int teacherId, const QString& teacherName,
                             QWidget *parent)
    : QMainWindow(parent), db_(db), teacherId_(teacherId), teacherName_(teacherName) {

    setupUI();
    setWindowTitle(QString("Преподаватель: %1").arg(teacherName_));
    resize(1000, 700);
}

TeacherWindow::~TeacherWindow() {
}

void TeacherWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(teacherName_), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QLabel* statusLabel = new QLabel("🚧 Окно преподавателя в разработке...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(12);
    statusLabel->setFont(statusFont);
    mainLayout->addWidget(statusLabel);

    mainLayout->addStretch();
}
