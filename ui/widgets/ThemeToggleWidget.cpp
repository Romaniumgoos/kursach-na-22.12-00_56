#include "ThemeToggleWidget.h"
#include <QVBoxLayout>
#include <QApplication>

ThemeToggleWidget::ThemeToggleWidget(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);

    m_toggleButton = new QPushButton("🌙 Тёмная тема");
    m_toggleButton->setMaximumWidth(150);
    layout->addWidget(m_toggleButton);
    layout->addStretch();

    connect(m_toggleButton, &QPushButton::clicked, this, &ThemeToggleWidget::onToggleTheme);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ThemeToggleWidget::onThemeChanged);

    updateButtonText();
}

void ThemeToggleWidget::onToggleTheme()
{
    auto manager = ThemeManager::instance();
    ThemeManager::Theme newTheme = (manager->currentTheme() == ThemeManager::Dark)
                                       ? ThemeManager::Light
                                       : ThemeManager::Dark;
    manager->setTheme(newTheme);
    ThemeManager::instance()->applyTheme(qApp);
}

void ThemeToggleWidget::onThemeChanged(ThemeManager::Theme theme)
{
    updateButtonText();
}

void ThemeToggleWidget::updateButtonText()
{
    if (ThemeManager::instance()->currentTheme() == ThemeManager::Dark) {
        m_toggleButton->setText("☀️ Светлая тема");
    } else {
        m_toggleButton->setText("🌙 Тёмная тема");
    }
}
