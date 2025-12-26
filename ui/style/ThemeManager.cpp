#include "ui/style/ThemeManager.h"

#include <QSettings>
#include <QPalette>
#include <QColor>
#include <QStyle>
#include <QFile>
#include <QTextStream>

ThemeManager* ThemeManager::m_instance = nullptr;

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    loadSetting();
}

ThemeManager* ThemeManager::instance()
{
    if (!m_instance) m_instance = new ThemeManager();
    return m_instance;
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme == theme) return;
    m_currentTheme = theme;
    saveSetting();
    emit themeChanged(theme);
}

void ThemeManager::applyTheme(QApplication* app)
{
    app->setStyle("Fusion");

    QPalette pal;

    if (m_currentTheme == Dark) {
        // 3 слоя фона: Window (самый тёмный) -> Base (панели) -> AlternateBase (карточки)
        pal.setColor(QPalette::Window, QColor(18, 18, 18));
        pal.setColor(QPalette::Base, QColor(27, 27, 27));
        pal.setColor(QPalette::AlternateBase, QColor(36, 36, 36));

        pal.setColor(QPalette::Mid, QColor(150, 150, 150));
        pal.setColor(QPalette::Midlight, QColor(185, 185, 185));

        pal.setColor(QPalette::WindowText, QColor(232, 234, 237));
        pal.setColor(QPalette::Text, QColor(232, 234, 237));

        pal.setColor(QPalette::ToolTipBase, QColor(36, 36, 36));
        pal.setColor(QPalette::ToolTipText, QColor(232, 234, 237));

        pal.setColor(QPalette::Button, QColor(36, 36, 36));
        pal.setColor(QPalette::ButtonText, QColor(232, 234, 237));

        pal.setColor(QPalette::BrightText, QColor(255, 99, 99));
        pal.setColor(QPalette::Link, QColor(111, 183, 255));

        // Highlight делаем не слишком "ядовитым", и чтобы текст на нём читался
        pal.setColor(QPalette::Highlight, QColor(74, 163, 255));
        pal.setColor(QPalette::HighlightedText, QColor(11, 15, 20));

        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(150, 150, 150));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150));
    } else {
        // Реально светлая палитра (белая)
        pal.setColor(QPalette::Window, QColor(250, 250, 250));
        pal.setColor(QPalette::WindowText, QColor(20, 20, 20));

        pal.setColor(QPalette::Base, Qt::white);
        pal.setColor(QPalette::AlternateBase, QColor(245, 245, 245));

        pal.setColor(QPalette::Mid, QColor(110, 110, 110));
        pal.setColor(QPalette::Midlight, QColor(140, 140, 140));

        pal.setColor(QPalette::ToolTipBase, Qt::white);
        pal.setColor(QPalette::ToolTipText, QColor(20, 20, 20));

        pal.setColor(QPalette::Text, QColor(20, 20, 20));
        pal.setColor(QPalette::Button, QColor(245, 245, 245));
        pal.setColor(QPalette::ButtonText, QColor(20, 20, 20));

        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, QColor(0, 102, 204));

        pal.setColor(QPalette::Highlight, QColor(76, 163, 224));
        pal.setColor(QPalette::HighlightedText, Qt::white);
    }


    app->setPalette(pal);
    QFile f(":/style/app.qss");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        app->setStyleSheet(in.readAll());
    } else {
        // fallback: если не подключили ресурсы, попробуем файл по пути проекта
        QFile f2("ui/style/app.qss");
        if (f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in2(&f2);
            app->setStyleSheet(in2.readAll());
        }
    }

}

void ThemeManager::saveSetting()
{
    QSettings s("SchoolApp", "StudentManagementSystem");
    s.setValue("theme", static_cast<int>(m_currentTheme));
}

void ThemeManager::loadSetting()
{
    QSettings s("SchoolApp", "StudentManagementSystem");
    const int v = s.value("theme", static_cast<int>(Dark)).toInt();
    m_currentTheme = static_cast<Theme>(v);
}
