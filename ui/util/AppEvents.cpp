#include "ui/util/AppEvents.h"

AppEvents& AppEvents::instance()
{
    static AppEvents inst;
    return inst;
}

AppEvents::AppEvents(QObject* parent)
    : QObject(parent)
{
}

void AppEvents::emitScheduleChanged()
{
    emit scheduleChanged();
}
