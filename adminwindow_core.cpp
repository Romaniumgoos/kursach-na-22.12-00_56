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
