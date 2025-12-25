#include "ui/widgets/LessonCardWidget.h"

 #include "ui/util/UiStyle.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

QString LessonCardWidget::stripeColorCss(const QString& lessonType)
{
    if (lessonType.contains("ЛР")) return "#ff9150";
    if (lessonType.contains("ПЗ")) return "#46aaff";
    if (lessonType.contains("ЛК")) return "#aa78ff";
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

    auto* titleLabel = new QLabel(subject, body);
    titleLabel->setObjectName("LessonCardTitle");
    titleLabel->setWordWrap(true);
    topRow->addWidget(titleLabel, 1);

    if (!lessonType.isEmpty()) {
        auto* typeBadge = new QLabel(lessonType, body);
        typeBadge->setAlignment(Qt::AlignCenter);
        typeBadge->setMinimumHeight(22);
        typeBadge->setStyleSheet(UiStyle::badgeLessonTypeStyle(lessonType));
        topRow->addWidget(typeBadge, 0);
    }

    v->addLayout(topRow);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(6);

    auto* roomBadge = new QLabel(room.isEmpty() ? QString("—") : room, body);
    roomBadge->setAlignment(Qt::AlignCenter);
    roomBadge->setMinimumHeight(22);
    roomBadge->setStyleSheet(UiStyle::badgeNeutralStyle());
    bottomRow->addWidget(roomBadge, 0);

    auto* teacherPill = new QLabel(teacher, body);
    teacherPill->setObjectName("LessonCardTeacher");
    teacherPill->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    teacherPill->setWordWrap(false);
    bottomRow->addWidget(teacherPill, 1);

    if (subgroup == 1 || subgroup == 2) {
        auto* sg = new QLabel(QString::number(subgroup), body);
        sg->setObjectName("LessonCardSubgroup");
        sg->setAlignment(Qt::AlignCenter);
        sg->setMinimumHeight(22);
        sg->setStyleSheet(UiStyle::badgeNeutralStyle());
        bottomRow->addWidget(sg, 0, Qt::AlignRight);
    }

    v->addLayout(bottomRow);
}
