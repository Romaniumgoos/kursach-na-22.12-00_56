#pragma once

#include <QWidget>

class Database;
class QLabel;
class QComboBox;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class StudentGradesPage : public QWidget {
    Q_OBJECT
public:
    explicit StudentGradesPage(Database* db, int studentId, QWidget* parent = nullptr);

public slots:
    void reload();

private:
    Database* db;
    int studentId;

    int semesterId;

    QComboBox* semesterCombo;
    QPushButton* refreshButton;

    QStackedWidget* stacked;
    QWidget* contentWidget;
    QWidget* emptyWidget;

    QTableWidget* table;
    QLabel* averageLabel;

    QLabel* emptyStateLabel;
    QPushButton* retryButton;

    void setupLayout();
    void setupTable();
    void populateSemesters();
    void showEmptyState(const QString& message);
    void showContent();
};
