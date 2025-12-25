#pragma once

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QPalette>
#include <QTableWidget>

namespace UiStyle {

inline constexpr const char* kInfoLabelStyle =
    "QLabel {"
    "  padding: 10px 12px;"
    "  border-radius: 8px;"
    "  background: rgba(0,0,0,0.04);"
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

} // namespace UiStyle
