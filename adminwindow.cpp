#include "adminwindow.h"
#include <QVBoxLayout>
#include <QFont>
#include "ui/widgets/ThemeToggleWidget.h"

AdminWindow::AdminWindow(Database* db, int adminId, const QString& adminName,
                         QWidget *parent)
    : QMainWindow(parent), db(db), adminId(adminId), adminName(adminName) {

    setupUI();
    setWindowTitle(QString("Администратор: %1").arg(adminName));
    resize(1100, 750);
}

AdminWindow::~AdminWindow() {
}

void AdminWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(adminName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QLabel* statusLabel = new QLabel("🚧 Окно администратора в разработке...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    QFont statusFont = statusLabel->font();
    statusFont.setPointSize(12);
    statusLabel->setFont(statusFont);
    mainLayout->addWidget(statusLabel);

    mainLayout->addStretch();
}
