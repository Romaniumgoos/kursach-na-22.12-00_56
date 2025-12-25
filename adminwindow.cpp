#include "adminwindow.h"
#include <QVBoxLayout>
#include <QFont>
#include "ui/widgets/ThemeToggleWidget.h"
 #include <QToolBar>
 #include <QAction>
 #include "loginwindow.h"

AdminWindow::AdminWindow(Database* db, int adminId, const QString& adminName,
                         QWidget *parent)
    : QMainWindow(parent), db(db), adminId(adminId), adminName(adminName) {

    setupUI();

    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

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
