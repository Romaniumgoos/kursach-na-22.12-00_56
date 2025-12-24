#include <QApplication>

#include "config.h"
#include "database.h"
#include "loginwindow.h"
#include "ui/style/ThemeManager.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Тема
    ThemeManager::instance()->applyTheme(&app);

    // БД всегда в корне проекта
    Database db(PROJECT_ROOT + "\\school.db");
    if (!db.connect()) return 1;
    if (!db.initialize()) return 1;

    LoginWindow window(&db);
    window.show();

    return app.exec();
}
