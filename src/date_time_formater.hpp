#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>


/// Provides date-and-time formatting for use by the application interface.
class DateTimeFormater : public QObject {
  Q_OBJECT
public:
  /// Creates a formatter with the given QObject parent.
  /// \param parent QObject that owns this formatter, or `nullptr`.
  explicit DateTimeFormater(QObject* parent = nullptr);

  /// Formats a date-time value as a localized time string.
  /// \param date_time Date and time to format.
  /// \return The formatted time.
  Q_INVOKABLE QString formatTime(const QDateTime& date_time) const;
};
