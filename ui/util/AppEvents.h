#pragma once

#include <QObject>

class AppEvents : public QObject {
    Q_OBJECT
public:
    static AppEvents& instance();

    void emitScheduleChanged();

signals:
    void scheduleChanged();

private:
    explicit AppEvents(QObject* parent = nullptr);
};
