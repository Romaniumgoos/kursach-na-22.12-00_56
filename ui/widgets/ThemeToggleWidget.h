#pragma once

#include <QWidget>
#include <QToolButton>
#include "ui/style/ThemeManager.h"

class ThemeToggleWidget : public QWidget {
    Q_OBJECT
public:
    explicit ThemeToggleWidget(QWidget* parent = nullptr);

private slots:
    void onToggled(bool checked);
    void onThemeChanged(ThemeManager::Theme theme);

private:
    QToolButton* btn_ = nullptr;
    void syncFromTheme();
};
