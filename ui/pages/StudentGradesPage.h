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
class QString;

class StudentGradesPage : public QWidget {
    Q_OBJECT
public:
    explicit StudentGradesPage(Database* db, int studentId, QWidget* parent = nullptr);

public slots:
    void reload();

private:
    struct GradeRow {
        std::string subject;
        int value = 0;
        std::string date;
        std::string type;
        std::string lessonTime;
        std::string lessonType;
    };

    Database* db;
    int studentId;

    int semesterId;

    QComboBox* semesterCombo;
    QComboBox* subjectCombo;
    QPushButton* refreshButton;

    QStackedWidget* stacked;
    QWidget* contentWidget;
    QWidget* emptyWidget;

    QScrollArea* listScroll;
    QWidget* listContainer;
    QVBoxLayout* listLayout;
    QLabel* averageLabel;
    QLabel* countLabel;

    QLabel* emptyStateLabel;
    QPushButton* retryButton;

    void setupLayout();
    void populateSemesters();
    void showEmptyState(const QString& message);
    void showContent();

    std::vector<GradeRow> cachedGrades;
    void populateSubjectsFromCache();
    void applyFilterToTimeline();
    QWidget* buildGradeCard(const GradeRow& g);
};
