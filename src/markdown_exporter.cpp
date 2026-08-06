#include "markdown_exporter.hpp"
#include "chat_message.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

QString MarkdownExporter::sanitizeTitle(const QString& title) {
  QString result;
  const QString simplified = title.simplified();
  for (int i = 0; i < simplified.length(); ++i) {
    const QChar character = simplified.at(i);
    if (character.isLetterOrNumber()) {
      result += character;
    } else if (not result.isEmpty() and not result.endsWith('_')) {
      result += '_';
    }
  }
  while (result.endsWith('_'))
    result.chop(1);
  return result.isEmpty() ? QString("chat") : result.left(50);
}

QString MarkdownExporter::outputPath(const QString& outputDirectory, const QString& title,
                                    const QDateTime& timestamp) {
  const QDir directory(outputDirectory);
  const QString base = sanitizeTitle(title) + "_" + timestamp.toString("yyyyMMdd_HHmmss");
  QString path = directory.absoluteFilePath(base + ".md");
  int suffix = 2;
  while (QFile::exists(path)) {
    path = directory.absoluteFilePath(base + QString("_%1.md").arg(suffix++));
  }
  return path;
}

MarkdownExporter::Result MarkdownExporter::exportMessages(const QList<ChatMessage*>& messages,
                                                           const QString& outputDirectory) {
  Result result = {false, QString(), QString()};

  QList<ChatMessage*> exportMessages;
  bool hasUserMessage = false;
  for (int i = 0; i < messages.size(); ++i) {
    ChatMessage* message = messages.at(i);
    if (not message or message->role() == ChatMessage::System)
      continue;
    if (message->role() == ChatMessage::User and not message->content().trimmed().isEmpty())
      hasUserMessage = true;
    exportMessages.append(message);
  }
  if (not hasUserMessage) {
    result.error = "A conversation must contain a user message before it can be exported";
    return result;
  }

  QString directoryPath = outputDirectory;
  if (directoryPath.isEmpty()) {
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty()) {
      result.error = "Documents location not found";
      return result;
    }
    directoryPath = QDir(documents).absoluteFilePath("SailPrompt");
  }

  QFileInfo destination(directoryPath);
  if (destination.exists() and not destination.isDir()) {
    result.error = "The Markdown destination is not a directory";
    return result;
  }
  QDir directory;
  if (not directory.mkpath(directoryPath)) {
    result.error = "Could not create the Markdown destination directory";
    return result;
  }

  QString title;
  for (int i = 0; i < messages.size(); ++i) {
    if (messages.at(i) and messages.at(i)->role() == ChatMessage::User) {
      title = messages.at(i)->content();
      break;
    }
  }
  const QString path = outputPath(directoryPath, title);

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    result.error = "Failed to create the Markdown file";
    return result;
  }
  QTextStream out(&file);
  out.setCodec("UTF-8");

  for (int i = 0; i < exportMessages.size(); ++i) {
    ChatMessage* m = exportMessages.at(i);
    if (m->role() == ChatMessage::User) {
      out << "### User\n\n" << m->content() << "\n\n";
    } else if (m->role() == ChatMessage::Agent) {
      out << "### Assistant\n\n" << m->content() << "\n\n";
    }
  }
  out.flush();
  file.close();

  QFile verify(path);
  if (!verify.exists() || verify.size() == 0) {
    result.error = "Failed to write Markdown file";
    return result;
  }

  result.success = true;
  result.path = path;
  return result;
}
