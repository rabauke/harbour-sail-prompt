#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>


class DateTimeFormater : public QObject {
  Q_OBJECT
public:
  explicit DateTimeFormater(QObject* parent = nullptr);

  Q_INVOKABLE QString formatTime(const QDateTime& date_time) const;
};
