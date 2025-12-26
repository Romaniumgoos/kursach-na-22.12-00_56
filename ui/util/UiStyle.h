#pragma once

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPalette>
#include <QTableWidget>
 #include <QString>

namespace UiStyle {

inline constexpr const char* kInfoLabelStyle =
    "QLabel {"
    "  padding: 10px 12px;"
    "  border-radius: 8px;"
    "  background: rgba(120,120,120,0.10);"
    "  color: palette(WindowText);"
    "  font-weight: 600;"
    "}";

inline void applyStandardTableStyle(QTableWidget* table)
{
    if (!table) return;

    table->setAlternatingRowColors(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setWordWrap(false);
    table->setShowGrid(false);
    table->verticalHeader()->setVisible(false);

    auto* h = table->horizontalHeader();
    h->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    h->setSectionResizeMode(QHeaderView::Interactive);
    h->setStretchLastSection(true);
}

inline void makeInfoLabel(QLabel* label)
{
    if (!label) return;

    label->setStyleSheet(kInfoLabelStyle);
}

inline QString badgeStyle(const QString& borderRgba, const QString& bgRgba, int fontWeight)
{
    return QString("padding: 2px 10px; border-radius: 11px; border: 1px solid %1; background: %2; color: palette(WindowText); font-weight: %3;")
        .arg(borderRgba)
        .arg(bgRgba)
        .arg(fontWeight);
}

inline QString badgeNeutralStyle()
{
    return badgeStyle("rgba(120,120,120,0.42)", "rgba(120,120,120,0.10)", 600);
}

inline QString badgeLessonTypeStyle(const QString& lessonType)
{
    const QString t = lessonType.trimmed().toUpper();
    if (t.contains("ЛК")) {
        return badgeStyle("rgba(168,85,247,0.65)", "rgba(168,85,247,0.22)", 700);
    }
    if (t.contains("ЛБ")) {
        return badgeStyle("rgba(59,130,246,0.65)", "rgba(59,130,246,0.22)", 700);
    }
    if (t.contains("ПР") || t.contains("ПЗ")) {
        return badgeStyle("rgba(34,197,94,0.62)", "rgba(34,197,94,0.20)", 700);
    }
    return badgeStyle("rgba(249,115,22,0.68)", "rgba(249,115,22,0.20)", 700);
}

inline QString badgeGradeStyle(int value)
{
    if (value >= 9) {
        return badgeStyle("rgba(34,197,94,0.62)", "rgba(34,197,94,0.20)", 700);
    }
    if (value >= 7) {
        return badgeStyle("rgba(59,130,246,0.65)", "rgba(59,130,246,0.22)", 700);
    }
    if (value >= 5) {
        return badgeStyle("rgba(249,115,22,0.68)", "rgba(249,115,22,0.20)", 700);
    }
    return badgeStyle("rgba(239,68,68,0.62)", "rgba(239,68,68,0.20)", 700);
}

inline QString badgeAbsenceStyle(bool excused)
{
    return excused
        ? badgeStyle("rgba(34,197,94,0.62)", "rgba(34,197,94,0.20)", 600)
        : badgeStyle("rgba(239,68,68,0.62)", "rgba(239,68,68,0.20)", 600);
}

} // namespace UiStyle
