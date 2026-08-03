#pragma once

#include <QDateTime>
#include <QList>
#include <QString>


class ChatMessage;


class PdfExporter {
public:
  struct Result {
    bool success;
    QString path;
    QString error;
  };

  static QString sanitizeTitle(const QString& title);
  static QString outputPath(const QString& outputDirectory, const QString& title,
                            const QDateTime& timestamp = QDateTime::currentDateTime());
  static QString htmlForMessages(const QList<ChatMessage*>& messages, QString* error = nullptr);
  static Result exportMessages(const QList<ChatMessage*>& messages,
                               const QString& outputDirectory = QString());
};
