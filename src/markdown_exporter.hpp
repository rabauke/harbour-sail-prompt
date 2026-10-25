#pragma once

#include <QDateTime>
#include <QList>
#include <QString>


class ChatMessage;


/// Exports conversation messages to Markdown files.
class MarkdownExporter {
public:
  /// Holds the result of a Markdown export operation.
  struct Result {
    /// Whether the export completed successfully.
    bool success;

    /// Path of the exported file when successful.
    QString path;

    /// Description of the failure when unsuccessful.
    QString error;
  };

  /// Converts a conversation title into a filename-safe component.
  /// \param title Source title.
  /// \return Sanitized title, or `chat` if no usable characters remain.
  static QString sanitizeTitle(const QString& title);

  /// Builds an available Markdown file path from a directory, title, and timestamp.
  /// \param outputDirectory Destination directory.
  /// \param title Conversation title used in the filename.
  /// \param timestamp Timestamp used in the filename.
  /// \return Absolute path with a unique filename.
  static QString outputPath(const QString& outputDirectory, const QString& title,
                            const QDateTime& timestamp = QDateTime::currentDateTime());

  /// Writes the non-system messages to a Markdown file.
  /// The default destination is a `SailPrompt` directory under the user's Documents location.
  /// \param messages Conversation messages to export.
  /// \param outputDirectory Destination directory, or empty to use the default.
  /// \return Export status, output path, and any error description.
  static Result exportMessages(const QList<ChatMessage*>& messages,
                               const QString& outputDirectory = QString{});
};
