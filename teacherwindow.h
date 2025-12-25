#ifndef TEACHERWINDOW_H
#define TEACHERWINDOW_H

#include <QMainWindow>
#include "database.h"

class QLabel;
class QTabWidget;
class QComboBox;
class QTableWidget;
class QSpinBox;
class QPushButton;
class QStackedWidget;
class QDateEdit;
class QFrame;
class QScrollArea;
class QVBoxLayout;
class QEvent;

class PeriodSelectorWidget;
class WeekGridScheduleWidget;
struct WeekSelection;

class TeacherWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit TeacherWindow(Database* db, int teacherId, const QString& teacherName, 
                          QWidget *parent = nullptr);
    ~TeacherWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Database* db;
    int teacherId;
    QString teacherName;

    // Common
    QTabWidget* tabWidget = nullptr;

    // Schedule tab
    PeriodSelectorWidget* schedulePeriodSelector = nullptr;
    QComboBox* scheduleGroupCombo = nullptr;
    QComboBox* scheduleSubgroupCombo = nullptr;
    WeekGridScheduleWidget* scheduleGrid = nullptr;
    QLabel* scheduleEmptyLabel = nullptr;

    // Journal tab
    PeriodSelectorWidget* journalPeriodSelector = nullptr;
    QComboBox* journalSemesterCombo = nullptr;
    QComboBox* journalGroupCombo = nullptr;
    QComboBox* journalModeCombo = nullptr;
    QStackedWidget* journalModeStack = nullptr;
    QFrame* journalLessonCardFrame = nullptr;
    QLabel* journalLessonCardTitle = nullptr;
    QLabel* journalLessonCardDate = nullptr;
    QLabel* journalLessonCardWeekday = nullptr;
    QLabel* journalLessonCardTime = nullptr;
    QLabel* journalLessonCardNumber = nullptr;
    QLabel* journalLessonCardSubject = nullptr;
    QLabel* journalLessonCardType = nullptr;
    QLabel* journalLessonCardRoom = nullptr;
    QLabel* journalLessonCardGroup = nullptr;

    QTableWidget* journalStudentsTable = nullptr;

    QScrollArea* journalLessonsScroll = nullptr;
    QWidget* journalLessonsContainer = nullptr;
    QVBoxLayout* journalLessonsLayout = nullptr;

    struct JournalLessonRow {
        SelectedLesson lesson;
        QString weekdayName;
        QString timeText;
    };
    std::vector<JournalLessonRow> journalLessonRows;
    QSpinBox* gradeSpin = nullptr;
    QPushButton* saveGradeButton = nullptr;
    QPushButton* deleteGradeButton = nullptr;
    QLabel* currentGradeLabel = nullptr;
    QSpinBox* absenceHoursSpin = nullptr;
    QComboBox* absenceTypeCombo = nullptr;
    QPushButton* saveAbsenceButton = nullptr;
    QPushButton* deleteAbsenceButton = nullptr;
    QLabel* currentAbsenceLabel = nullptr;

    // Journal free mode (any student, absences only)
    QComboBox* freeGroupCombo = nullptr;
    QComboBox* freeStudentCombo = nullptr;
    QComboBox* freeSubjectCombo = nullptr;
    QDateEdit* freeDateEdit = nullptr;
    QSpinBox* freeHoursSpin = nullptr;
    QComboBox* freeTypeCombo = nullptr;
    QPushButton* freeSaveAbsenceButton = nullptr;

    // Group stats tab
    QComboBox* statsGroupCombo = nullptr;
    QComboBox* statsSemesterCombo = nullptr;
    QComboBox* statsSubjectCombo = nullptr;
    QTableWidget* statsGradesTable = nullptr;
    QTableWidget* statsAbsencesTable = nullptr;
    QLabel* statsGradesSummaryLabel = nullptr;
    QLabel* statsAbsencesSummaryLabel = nullptr;

    QWidget* statsDetailWidget = nullptr;
    QLabel* statsDetailTitleLabel = nullptr;
    QTableWidget* statsDetailGradesTable = nullptr;
    QTableWidget* statsDetailAbsencesTable = nullptr;
    QLabel* statsDetailGradesSummaryLabel = nullptr;
    QLabel* statsDetailAbsencesSummaryLabel = nullptr;

    struct SelectedLesson {
        bool valid = false;
        int scheduleId = 0;
        int subjectId = 0;
        int weekday = 0;
        int lessonNumber = 0;
        int subgroup = 0;
        QString subjectName;
        QString room;
        QString lessonType;
        QString dateISO;
    };
    SelectedLesson selectedLesson;

    void setupUI();
    QWidget* buildScheduleTab();
    QWidget* buildJournalTab();
    QWidget* buildGroupStatsTab();

    void loadTeacherGroups();
    void reloadSchedule();
    void reloadJournalStudents();
    void reloadJournalLessonsForSelectedStudent();
    void reloadGroupStats();
    void reloadGroupStatsSubjects();
    void reloadStatsStudentDetails(int studentId);

    bool resolveWeekSelection(const WeekSelection& selection, int& outResolvedWeekId, int& outWeekOfCycle, QString& outErrorText);
    QString formatDdMm(const QString& dateISO) const;

private slots:
    void onSchedulePeriodChanged(const WeekSelection& selection);
    void onScheduleGroupChanged(int);
    void onScheduleSubgroupChanged(int);

    void onJournalPeriodChanged(const WeekSelection& selection);
    void onJournalGroupChanged(int);
    void onJournalStudentSelectionChanged();
    void onJournalLessonCardClicked(int index);
    void onSaveGrade();
    void onDeleteGrade();
    void onSaveAbsence();
    void onDeleteAbsence();

    void onJournalModeChanged(int);
    void onFreeGroupChanged(int);
    void onFreeSaveAbsence();

    void onStatsGroupChanged(int);
    void onStatsSemesterChanged(int);
    void onStatsSubjectChanged(int);
    void onStatsStudentSelected();

private:
    void refreshJournalCurrentValues();
    void openGradeDialog();
    void openAbsenceDialog();
};

#endif // TEACHERWINDOW_H
