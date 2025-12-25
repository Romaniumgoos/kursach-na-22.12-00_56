#pragma once

#include <QWidget>
#include <QString>

class LessonCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit LessonCardWidget(const QString& subject,
                              const QString& room,
                              const QString& lessonType,
                              const QString& teacher,
                              int subgroup,
                              QWidget* parent = nullptr);

private:
    static QString stripeColorCss(const QString& lessonType);
};
