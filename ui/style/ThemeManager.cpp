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
        pal.setColor(QPalette::Window, QColor(53, 53, 53));
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Base, QColor(25, 25, 25));
        pal.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        pal.setColor(QPalette::ToolTipBase, Qt::white);
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Button, QColor(53, 53, 53));
        pal.setColor(QPalette::ButtonText, Qt::white);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, QColor(42, 130, 218));
        pal.setColor(QPalette::Highlight, QColor(42, 130, 218));
        pal.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        // Реально светлая палитра (белая)
        pal.setColor(QPalette::Window, QColor(250, 250, 250));
        pal.setColor(QPalette::WindowText, QColor(20, 20, 20));

        pal.setColor(QPalette::Base, Qt::white);
        pal.setColor(QPalette::AlternateBase, QColor(245, 245, 245));

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
