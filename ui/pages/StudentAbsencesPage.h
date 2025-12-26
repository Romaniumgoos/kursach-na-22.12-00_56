#pragma once

#include <QWidget>

#include <vector>
#include <string>

class Database;
class QLabel;
class QComboBox;
class QPushButton;
class QStackedWidget;
class QScrollArea;
class QVBoxLayout;

class StudentAbsencesPage : public QWidget {
    Q_OBJECT
public:
    explicit StudentAbsencesPage(Database* db, int studentId, QWidget* parent = nullptr);

public slots:
    void reload();

private:
    struct AbsenceRow {
        std::string subject;
        int hours = 0;
        std::string date;
        std::string type;
        std::string lessonTime;
        std::string lessonType;
    };

    Database* db;
    int studentId;

    int semesterId;

    int selectedYear = 0;
    int selectedMonth = 0;

    enum class FilterMode {
        All,
        Excused,
        Unexcused
    };
    FilterMode filterMode;

    QComboBox* semesterCombo;
    QComboBox* filterCombo;
    QComboBox* monthCombo;
    QComboBox* subjectCombo;
    QPushButton* refreshButton;

    QStackedWidget* stacked;
    QWidget* contentWidget;
    QWidget* emptyWidget;

    QScrollArea* listScroll;
    QWidget* listContainer;
    QVBoxLayout* listLayout;
    QLabel* totalAbsencesLabel;
    QLabel* unexcusedAbsencesLabel;

    QLabel* absSubjectMonthLabel;
    QLabel* absAllMonthLabel;

    QLabel* emptyStateLabel;
    QPushButton* retryButton;

    std::vector<AbsenceRow> cachedAbsences;

    void setupLayout();

    void populateSemesters();
    void showEmptyState(const QString& message);
    void showContent();

    void applyFilterToTimeline();
    void updateTotalsFromCache();
    void populateSubjectsFromCache();
    void populateMonthsFromCache();
    void updateMonthlyStats();
    QWidget* buildAbsenceCard(const AbsenceRow& r);

    int subjectIdForCurrentFilter() const;
};
