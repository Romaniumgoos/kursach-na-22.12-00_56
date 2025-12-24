#pragma once

#include <QObject>
#include <QApplication>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum Theme { Light, Dark };

    static ThemeManager* instance();

    Theme currentTheme() const { return m_currentTheme; }
    void setTheme(Theme theme);
    void applyTheme(QApplication* app);

    signals:
        void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject* parent = nullptr);

    void saveSetting();
    void loadSetting();

    static ThemeManager* m_instance;
    Theme m_currentTheme = Dark;
};
