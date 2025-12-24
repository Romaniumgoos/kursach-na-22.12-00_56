#include "studentwindow.h"
#include "statistics.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include "ui/widgets/ThemeToggleWidget.h"
#include <QToolBar>

StudentWindow::StudentWindow(Database* db, int studentId, const QString& studentName, 
                             QWidget *parent)
    : QMainWindow(parent), db_(db), studentId_(studentId), 
      studentName_(studentName), groupId_(0), subgroup_(0) {

    loadStudentInfo();
    setupUI();
    auto* tb = new QToolBar("Toolbar", this);
    tb->setMovable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    themeToggle_ = new ThemeToggleWidget(this);
    tb->addWidget(themeToggle_);


    setWindowTitle(QString("Студент: %1").arg(studentName_));
    resize(900, 600);

    // Загрузить данные
    loadGrades();
    loadAbsences();
    loadSchedule();
}

StudentWindow::~StudentWindow() {
}

void StudentWindow::loadStudentInfo() {
    db_->getStudentGroupAndSubgroup(studentId_, groupId_, subgroup_);
}

void StudentWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Заголовок
    QLabel* titleLabel = new QLabel(QString("Добро пожаловать, %1!").arg(studentName_), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Вкладки
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    setupGradesTab();
    setupAbsencesTab();
    setupScheduleTab();
}

void StudentWindow::setupGradesTab() {
    gradesTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(gradesTab_);

    // Таблица оценок
    gradesTable_ = new QTableWidget(gradesTab_);
    gradesTable_->setColumnCount(4);
    gradesTable_->setHorizontalHeaderLabels({"Предмет", "Оценка", "Дата", "Тип"});
    gradesTable_->horizontalHeader()->setStretchLastSection(true);
    gradesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gradesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(gradesTable_);

    // Средний балл
    averageLabel_ = new QLabel(gradesTab_);
    QFont avgFont = averageLabel_->font();
    avgFont.setPointSize(12);
    avgFont.setBold(true);
    averageLabel_->setFont(avgFont);
    layout->addWidget(averageLabel_);

    tabWidget_->addTab(gradesTab_, "📊 Оценки");
}

void StudentWindow::setupAbsencesTab() {
    absencesTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(absencesTab_);

    // Таблица пропусков
    absencesTable_ = new QTableWidget(absencesTab_);
    absencesTable_->setColumnCount(4);
    absencesTable_->setHorizontalHeaderLabels({"Дата", "Предмет", "Часы", "Тип"});
    absencesTable_->horizontalHeader()->setStretchLastSection(true);
    absencesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    absencesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(absencesTable_);

    // Итого пропусков
    totalAbsencesLabel_ = new QLabel(absencesTab_);
    QFont totalFont = totalAbsencesLabel_->font();
    totalFont.setPointSize(12);
    totalFont.setBold(true);
    totalAbsencesLabel_->setFont(totalFont);
    layout->addWidget(totalAbsencesLabel_);

    tabWidget_->addTab(absencesTab_, "❌ Пропуски");
}

void StudentWindow::setupScheduleTab() {
    scheduleTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(scheduleTab_);

    // Выбор недели
    QHBoxLayout* weekLayout = new QHBoxLayout();
    weekLayout->addWidget(new QLabel("Неделя:", scheduleTab_));

    weekComboBox_ = new QComboBox(scheduleTab_);

    // Загрузить список недель из cycleweeks
    std::vector<std::tuple<int, int, std::string, std::string>> weeks;
    if (db_->getCycleWeeks(weeks)) {
        for (const auto& week : weeks) {
            int weekId = std::get<0>(week);
            int weekOfCycle = std::get<1>(week);
            QString startDate = QString::fromStdString(std::get<2>(week));
            QString endDate = QString::fromStdString(std::get<3>(week));

            QString label = QString("Неделя %1 (цикл %2): %3 — %4")
                            .arg(weekId)
                            .arg(weekOfCycle)
                            .arg(startDate)
                            .arg(endDate);

            weekComboBox_->addItem(label, weekId);
        }
    }

    weekLayout->addWidget(weekComboBox_);
    weekLayout->addStretch();
    layout->addLayout(weekLayout);

    // Таблица расписания
    scheduleTable_ = new QTableWidget(scheduleTab_);
    scheduleTable_->setColumnCount(6);
    scheduleTable_->setHorizontalHeaderLabels({
        "День недели", "Пара", "Время", "Предмет", "Аудитория", "Преподаватель"
    });
    scheduleTable_->horizontalHeader()->setStretchLastSection(true);
    scheduleTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(scheduleTable_);

    tabWidget_->addTab(scheduleTab_, "📅 Расписание");

    // Подключить сигнал изменения недели
    connect(weekComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StudentWindow::onWeekChanged);
}

void StudentWindow::loadGrades() {
    int semesterId = 1; // Пока что фиксированный семестр

    std::vector<std::tuple<std::string, int, std::string, std::string>> grades;
    if (!db_->getStudentGradesForSemester(studentId_, semesterId, grades)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить оценки");
        return;
    }

    gradesTable_->setRowCount(0);

    for (const auto& grade : grades) {
        int row = gradesTable_->rowCount();
        gradesTable_->insertRow(row);

        QString subject = QString::fromStdString(std::get<0>(grade));
        int value = std::get<1>(grade);
        QString date = QString::fromStdString(std::get<2>(grade));
        QString type = QString::fromStdString(std::get<3>(grade));

        gradesTable_->setItem(row, 0, new QTableWidgetItem(subject));
        gradesTable_->setItem(row, 1, new QTableWidgetItem(QString::number(value)));
        gradesTable_->setItem(row, 2, new QTableWidgetItem(date));
        gradesTable_->setItem(row, 3, new QTableWidgetItem(type));
    }

    // Рассчитать средний балл
    double average = Statistics::calculateStudentAverage(*db_, studentId_, semesterId);
    averageLabel_->setText(QString("Средний балл: %1").arg(average, 0, 'f', 2));

    // Цвет в зависимости от среднего
    if (average >= 9.0) {
        averageLabel_->setStyleSheet("color: green;");
    } else if (average >= 7.0) {
        averageLabel_->setStyleSheet("color: blue;");
    } else if (average >= 5.0) {
        averageLabel_->setStyleSheet("color: orange;");
    } else {
        averageLabel_->setStyleSheet("color: red;");
    }
}

void StudentWindow::loadAbsences() {
    int semesterId = 1;

    std::vector<std::tuple<std::string, int, std::string, std::string>> absences;
    if (!db_->getStudentAbsencesForSemester(studentId_, semesterId, absences)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить пропуски");
        return;
    }

    absencesTable_->setRowCount(0);

    int totalHours = 0;
    for (const auto& absence : absences) {
        int row = absencesTable_->rowCount();
        absencesTable_->insertRow(row);

        QString subject = QString::fromStdString(std::get<0>(absence));
        int hours = std::get<1>(absence);
        QString date = QString::fromStdString(std::get<2>(absence));
        QString type = QString::fromStdString(std::get<3>(absence));

        totalHours += hours;

        QString typeRu = (type == "excused") ? "Уважительный" : "Неуважительный";

        absencesTable_->setItem(row, 0, new QTableWidgetItem(date));
        absencesTable_->setItem(row, 1, new QTableWidgetItem(subject));
        absencesTable_->setItem(row, 2, new QTableWidgetItem(QString::number(hours)));
        absencesTable_->setItem(row, 3, new QTableWidgetItem(typeRu));

        // Цвет строки
        QColor rowColor = (type == "excused") ? QColor(200, 255, 200) : QColor(255, 200, 200);
        for (int col = 0; col < 4; ++col) {
            absencesTable_->item(row, col)->setBackground(rowColor);
        }
    }

    totalAbsencesLabel_->setText(QString("Всего пропущено: %1 часов").arg(totalHours));
}

void StudentWindow::loadSchedule() {
    int weekId = weekComboBox_->currentData().toInt();
    if (weekId == 0) return;

    int weekOfCycle = db_->getWeekOfCycleByWeekId(weekId);
    if (weekOfCycle == 0) return;

    scheduleTable_->setRowCount(0);

    QStringList dayNames = {"Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
    QStringList pairTimes = {
        "08:30-09:55", "10:05-11:30", "12:00-13:25",
        "13:35-15:00", "15:30-16:55", "17:05-18:30"
    };

    for (int weekday = 0; weekday <= 5; ++weekday) {
        std::vector<std::tuple<int, int, int, std::string, std::string, std::string, std::string>> rows;

        if (!db_->getScheduleForGroup(groupId_, weekday, weekOfCycle, rows)) {
            continue;
        }

        for (const auto& row : rows) {
            int lessonNum = std::get<1>(row);
            int subgroupNum = std::get<2>(row);
            QString subject = QString::fromStdString(std::get<3>(row));
            QString room = QString::fromStdString(std::get<4>(row));
            QString lessonType = QString::fromStdString(std::get<5>(row));
            QString teacher = QString::fromStdString(std::get<6>(row));

            // Фильтр по подгруппе
            if (subgroupNum != 0 && subgroupNum != subgroup_) {
                continue;
            }

            int tableRow = scheduleTable_->rowCount();
            scheduleTable_->insertRow(tableRow);

            scheduleTable_->setItem(tableRow, 0, new QTableWidgetItem(dayNames[weekday]));
            scheduleTable_->setItem(tableRow, 1, new QTableWidgetItem(QString::number(lessonNum)));
            scheduleTable_->setItem(tableRow, 2, new QTableWidgetItem(pairTimes[lessonNum - 1]));

            QString subjectFull = QString("%1 (%2)").arg(subject).arg(lessonType);
            if (subgroupNum == 1) subjectFull += " [Подгр. 1]";
            if (subgroupNum == 2) subjectFull += " [Подгр. 2]";

            scheduleTable_->setItem(tableRow, 3, new QTableWidgetItem(subjectFull));
            scheduleTable_->setItem(tableRow, 4, new QTableWidgetItem(room));
            scheduleTable_->setItem(tableRow, 5, new QTableWidgetItem(teacher));
        }
    }
}

void StudentWindow::onWeekChanged(int index) {
    Q_UNUSED(index);
    loadSchedule();
}
