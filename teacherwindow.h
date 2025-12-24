#ifndef TEACHERWINDOW_H
#define TEACHERWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "database.h"

class TeacherWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit TeacherWindow(Database* db, int teacherId, const QString& teacherName, 
                          QWidget *parent = nullptr);
    ~TeacherWindow();

private:
    Database* db_;
    int teacherId_;
    QString teacherName_;

    void setupUI();
};

#endif // TEACHERWINDOW_H
