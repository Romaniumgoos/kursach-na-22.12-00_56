#include "ui/widgets/LessonCardWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

QString LessonCardWidget::stripeColorCss(const QString& lessonType)
{
    if (lessonType.contains("ЛР")) return "#e85d5d";
    if (lessonType.contains("ЛК")) return "#4fb06a";
    if (lessonType.contains("ПЗ")) return "#e6c14a";
    return "#9aa0a6";
}

LessonCardWidget::LessonCardWidget(const QString& subject,
                                 const QString& room,
                                 const QString& lessonType,
                                 const QString& teacher,
                                 int subgroup,
                                 QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setObjectName("LessonCard");

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* stripe = new QWidget(this);
    stripe->setFixedWidth(8);
    stripe->setObjectName("LessonCardStripe");
    stripe->setProperty("stripeColor", stripeColorCss(lessonType));
    root->addWidget(stripe);

    auto* body = new QWidget(this);
    body->setObjectName("LessonCardBody");
    root->addWidget(body);

    auto* v = new QVBoxLayout(body);
    v->setContentsMargins(10, 8, 16, 8);
    v->setSpacing(6);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(8);

    QString title = subject;
    if (!lessonType.isEmpty()) title += QString(" (%1)").arg(lessonType);

    auto* titleLabel = new QLabel(title, body);
    titleLabel->setObjectName("LessonCardTitle");
    titleLabel->setWordWrap(true);
    topRow->addWidget(titleLabel, 1);

    auto* roomLabel = new QLabel(room, body);
    roomLabel->setObjectName("LessonCardRoom");
    roomLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    topRow->addWidget(roomLabel, 0);

    v->addLayout(topRow);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(6);

    auto* teacherPill = new QLabel(teacher, body);
    teacherPill->setObjectName("LessonCardTeacher");
    teacherPill->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    teacherPill->setWordWrap(false);
    bottomRow->addWidget(teacherPill, 1);

    if (subgroup == 1 || subgroup == 2) {
        auto* sg = new QLabel(QString::number(subgroup), body);
        sg->setObjectName("LessonCardSubgroup");
        bottomRow->addWidget(sg, 0, Qt::AlignRight);
    }

    v->addLayout(bottomRow);
}
