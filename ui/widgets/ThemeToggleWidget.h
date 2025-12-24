#pragma once

#include <QWidget>
#include <QPushButton>
#include "ui/style/ThemeManager.h"

class ThemeToggleWidget : public QWidget {
    Q_OBJECT

public:
    explicit ThemeToggleWidget(QWidget* parent = nullptr);

private slots:
    void onToggleTheme();
    void onThemeChanged(ThemeManager::Theme theme);

private:
    QPushButton* m_toggleButton;
    void updateButtonText();
};
