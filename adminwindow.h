#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "database.h"

class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(Database* db, int adminId, const QString& adminName, 
                        QWidget *parent = nullptr);
    ~AdminWindow();

private:
    Database* db;
    int adminId;
    QString adminName;

    void setupUI();
};

#endif // ADMINWINDOW_H
