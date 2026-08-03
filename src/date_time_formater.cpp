#include "date_time_formater.hpp"
#include <QLocale>


DateTimeFormater::DateTimeFormater(QObject *parent) : QObject{parent} {
}


QString DateTimeFormater::formatTime(const QDateTime &date_time) const {
  QLocale locale;
  return locale.toString(date_time, QLocale::ShortFormat);
}
