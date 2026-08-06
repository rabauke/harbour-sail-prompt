#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

class ChatMessage;

class MarkdownExporter {
public:
  struct Result {
    bool success;
    QString path;
    QString error;
  };

  static QString sanitizeTitle(const QString& title);
  static QString outputPath(const QString& outputDirectory, const QString& title,
                            const QDateTime& timestamp = QDateTime::currentDateTime());
  static Result exportMessages(const QList<ChatMessage*>& messages,
                               const QString& outputDirectory = QString());
};
