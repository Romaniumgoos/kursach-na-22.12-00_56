#include "studentwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ui/widgets/ThemeToggleWidget.h"
#include <QToolBar>
#include <QAction>
#include "ui/widgets/PeriodSelectorWidget.h"
#include "ui/pages/StudentSchedulePage.h"
#include "ui/pages/StudentGradesPage.h"
#include "ui/pages/StudentAbsencesPage.h"
 #include "loginwindow.h"

 #include "ui/util/AppEvents.h"

StudentWindow::StudentWindow(Database* db, int studentId, const QString& studentName,
                             QWidget *parent)
    : QMainWindow(parent), db(db), studentId(studentId),
      studentName(studentName) {

    setupUI();
    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    themeToggle = new ThemeToggleWidget(this);
    tb->addWidget(themeToggle);

    auto* logoutAction = new QAction("Выйти в авторизацию", this);
    tb->addAction(logoutAction);
    connect(logoutAction, &QAction::triggered, this, [this]() {
        this->hide();
        auto* login = new LoginWindow(this->db);
        login->setAttribute(Qt::WA_DeleteOnClose);
        login->show();
        this->close();
    });


    setWindowTitle(QString("Студент: %1").arg(studentName));
    resize(900, 600);

    connect(&AppEvents::instance(), &AppEvents::scheduleChanged, this, [this]() {
        if (schedulePage && schedulePeriodSelector) {
            schedulePage->onPeriodChanged(schedulePeriodSelector->currentSelection());
        }
    });
}

StudentWindow::~StudentWindow() {
}

void StudentWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Заголовок
    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(studentName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Вкладки
    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    auto* gradesPage = new StudentGradesPage(db, studentId, this);
    auto* absencesPage = new StudentAbsencesPage(db, studentId, this);
    tabWidget->addTab(gradesPage, "📊 Оценки");
    tabWidget->addTab(absencesPage, "❌ Пропуски");

    setupScheduleTab();
}

void StudentWindow::setupScheduleTab() {
    auto* scheduleWidget = new QWidget();
    auto* scheduleLayout = new QVBoxLayout(scheduleWidget);

    schedulePeriodSelector = new PeriodSelectorWidget(db, scheduleWidget);
    schedulePage = new StudentSchedulePage(db, studentId, scheduleWidget);

    scheduleLayout->addWidget(schedulePeriodSelector);
    scheduleLayout->addWidget(schedulePage);

    connect(schedulePeriodSelector, &PeriodSelectorWidget::selectionChanged,
            schedulePage, &StudentSchedulePage::onPeriodChanged);

    // Инициализируем с текущим выбором
    schedulePage->onPeriodChanged(schedulePeriodSelector->currentSelection());

    tabWidget->addTab(scheduleWidget, "📅 Расписание");
}
