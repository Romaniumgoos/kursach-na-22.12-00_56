#pragma once

#include <QWidget>
#include <QString>

class QMouseEvent;

class LessonCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit LessonCardWidget(const QString& subject,
                              const QString& room,
                              const QString& lessonType,
                              const QString& teacher,
                              int subgroup,
                              QWidget* parent = nullptr);

    explicit LessonCardWidget(int scheduleId,
                              const QString& subject,
                              const QString& room,
                              const QString& lessonType,
                              const QString& teacher,
                              int subgroup,
                              QWidget* parent = nullptr);

signals:
    void clicked(int scheduleId);

private:
    static QString stripeColorCss(const QString& lessonType);

    int scheduleId = 0;

protected:
    void mousePressEvent(QMouseEvent* event) override;
};
