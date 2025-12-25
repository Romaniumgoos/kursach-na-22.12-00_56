#include "ui/widgets/ThemeToggleWidget.h"

#include <QHBoxLayout>
#include <QApplication>

ThemeToggleWidget::ThemeToggleWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    btn_ = new QToolButton(this);
    btn_->setCheckable(true);
    btn_->setAutoRaise(false);
    btn_->setCursor(Qt::PointingHandCursor);

    // “Дорогой” стиль toggle-пилюли
    btn_->setStyleSheet(R"(
        QToolButton {
            padding: 8px 14px;
            border-radius: 14px;
            border: 1px solid rgba(120,120,120,0.30);
            background: rgba(120,120,120,0.12);
            color: palette(WindowText);
            font-weight: 600;
        }
        QToolButton:hover {
            background: rgba(120,120,120,0.18);
        }
        QToolButton:checked {
            border: 1px solid rgba(80,160,220,0.55);
            background: rgba(80,160,220,0.28);
        }
        QToolButton:checked:hover {
            background: rgba(80,160,220,0.36);
        }
        QToolButton:focus {
            outline: none;
        }
    )");

    layout->addWidget(btn_);

    connect(btn_, &QToolButton::toggled, this, &ThemeToggleWidget::onToggled);
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ThemeToggleWidget::onThemeChanged);

    syncFromTheme();
}

void ThemeToggleWidget::syncFromTheme()
{
    const bool isDark = (ThemeManager::instance()->currentTheme() == ThemeManager::Dark);

    // checked=true будем трактовать как "Dark"
    btn_->blockSignals(true);
    btn_->setChecked(isDark);
    btn_->blockSignals(false);

    // Текст (и “иконка” юникодом)
    if (isDark) btn_->setText(QString::fromUtf8("🌙 Dark"));
    else        btn_->setText(QString::fromUtf8("☀ Light"));
}

void ThemeToggleWidget::onToggled(bool checked)
{
    auto* tm = ThemeManager::instance();
    tm->setTheme(checked ? ThemeManager::Dark : ThemeManager::Light);
    tm->applyTheme(qApp); // применяем на всё приложение [web:207]
    syncFromTheme();
}

void ThemeToggleWidget::onThemeChanged(ThemeManager::Theme)
{
    syncFromTheme();
}
