#include "teacherwindow.h"
 #include "ui/widgets/PeriodSelectorWidget.h"
 #include "ui/widgets/ThemeToggleWidget.h"
 #include "ui/widgets/WeekGridScheduleWidget.h"
 #include "ui/util/UiStyle.h"
 #include "ui/util/AppEvents.h"
 #include "loginwindow.h"

 #include <QVBoxLayout>
 #include <QHBoxLayout>
 #include <QToolBar>
 #include <QAction>
 #include <QTabWidget>
 #include <QComboBox>
 #include <QTableWidget>
 #include <QHeaderView>
 #include <QFont>
 #include <QItemSelectionModel>
 #include <QSpinBox>
 #include <QPushButton>
 #include <QMessageBox>
 #include <QStackedWidget>
 #include <QDateEdit>
 #include <QDate>
 #include <QAbstractItemView>
 #include <QFrame>
 #include <QGridLayout>
 #include <QGroupBox>
 #include <QDialog>
 #include <QDialogButtonBox>
 #include <QFormLayout>
 #include <QSplitter>
 #include <QScrollArea>
 #include <QStyle>
 #include <algorithm>
 #include <unordered_map>

 static QString lessonTypeBadgeStyle(const QString& lessonType)
 {
     return UiStyle::badgeLessonTypeStyle(lessonType);
 }

 int TeacherWindow::defaultSemesterId()
 {
     if (defaultSemesterResolved) return cachedDefaultSemesterId;
     defaultSemesterResolved = true;

     cachedDefaultSemesterId = 1;
     if (!db) return cachedDefaultSemesterId;

     std::vector<std::pair<int, std::string>> semesters;
     if (!db->getAllSemesters(semesters) || semesters.empty()) return cachedDefaultSemesterId;

     int best = 0;
     for (const auto& s : semesters) {
         if (s.first > best) best = s.first;
     }
     if (best > 0) cachedDefaultSemesterId = best;
     return cachedDefaultSemesterId;
 }

 static QString neutralBadgeStyle()
 {
     return UiStyle::badgeNeutralStyle();
 }

TeacherWindow::TeacherWindow(Database* db, int teacherId, const QString& teacherName,
                             QWidget *parent)
   : QMainWindow(parent), db(db), teacherId(teacherId), teacherName(teacherName) {

    setupUI();

    connect(&AppEvents::instance(), &AppEvents::scheduleChanged, this, [this]() {
        reloadSchedule();
    });

    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    auto* themeToggle = new ThemeToggleWidget(this);
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

    setWindowTitle(QString("Преподаватель: %1").arg(teacherName));
    resize(1000, 700);
}

TeacherWindow::~TeacherWindow() {
}

void TeacherWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(teacherName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    tabWidget->addTab(buildScheduleTab(), "📅 Расписание");
    tabWidget->addTab(buildJournalTab(), "📒 Журнал");
    tabWidget->addTab(buildGroupStatsTab(), "📊 Статистика");

    loadTeacherGroups();
}

void TeacherWindow::loadTeacherGroups()
{
    if (!db) return;
    std::vector<std::pair<int, std::string>> groups;
    std::vector<std::pair<int, std::string>> groupsFromSchedule;

    // 1) declared permissions (teachergroups)
    db->getGroupsForTeacher(teacherId, groups);

    // 2) factual assignments (schedule.teacherid) - required for teacher replacement scenario
    db->getGroupsFromScheduleForTeacher(teacherId, groupsFromSchedule);

    std::unordered_map<int, QString> byId;
    byId.reserve(groups.size() + groupsFromSchedule.size());
    for (const auto& g : groups) {
        byId[g.first] = QString::fromStdString(g.second);
    }
    for (const auto& g : groupsFromSchedule) {
        auto it = byId.find(g.first);
        if (it == byId.end() || it->second.trimmed().isEmpty()) {
            byId[g.first] = QString::fromStdString(g.second);
        }
    }

    groups.clear();
    groups.reserve(byId.size());
    for (const auto& kv : byId) {
        groups.emplace_back(kv.first, kv.second.toStdString());
    }
    std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    scheduleGroupCombo->blockSignals(true);
    journalGroupCombo->blockSignals(true);
    if (statsGroupCombo) statsGroupCombo->blockSignals(true);
    scheduleGroupCombo->clear();
    journalGroupCombo->clear();
    if (statsGroupCombo) statsGroupCombo->clear();
    scheduleGroupCombo->addItem("Все группы", 0);
    journalGroupCombo->addItem("Выберите группу", 0);
    if (statsGroupCombo) statsGroupCombo->addItem("Выберите группу", 0);

    for (const auto& g : groups) {
        const int id = g.first;
        const QString name = QString::fromStdString(g.second);
        scheduleGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
        journalGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
        if (statsGroupCombo) statsGroupCombo->addItem(name.isEmpty() ? QString("Группа %1").arg(id) : name, id);
    }

    scheduleGroupCombo->blockSignals(false);
    journalGroupCombo->blockSignals(false);
    if (statsGroupCombo) statsGroupCombo->blockSignals(false);

    // default: all groups
    reloadSchedule();

    reloadGroupStatsSubjects();
    reloadGroupStats();

}

QString TeacherWindow::formatDdMm(const QString& dateISO) const
{
    if (dateISO.size() >= 10) {
        return dateISO.mid(8, 2) + "." + dateISO.mid(5, 2);
    }
    return dateISO;
}
