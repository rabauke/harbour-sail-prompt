#include "pdf_exporter.hpp"
#include "chat_message.hpp"
#include "html_renderer.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPrinter>
#include <QStandardPaths>
#include <QTextDocument>


QString PdfExporter::sanitizeTitle(const QString& title) {
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


QString PdfExporter::outputPath(const QString& outputDirectory, const QString& title,
                                const QDateTime& timestamp) {
  const QDir directory(outputDirectory);
  const QString base = sanitizeTitle(title) + "_" + timestamp.toString("yyyyMMdd_HHmmss");
  QString path = directory.absoluteFilePath(base + ".pdf");
  int suffix = 2;
  while (QFile::exists(path)) {
    path = directory.absoluteFilePath(base + QString("_%1.pdf").arg(suffix++));
  }
  return path;
}


QString PdfExporter::htmlForMessages(const QList<ChatMessage*>& messages, QString* error) {
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
    if (error)
      *error = "A conversation must contain a user message before it can be exported";
    return QString();
  }
  if (error)
    error->clear();
  return HtmlRenderer::render(exportMessages);
}


PdfExporter::Result PdfExporter::exportMessages(const QList<ChatMessage*>& messages,
                                                const QString& outputDirectory) {
  Result result = {false, QString(), QString()};
  QString htmlError;
  const QString html = htmlForMessages(messages, &htmlError);
  if (html.isEmpty()) {
    result.error = htmlError;
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
    result.error = "The PDF destination is not a directory";
    return result;
  }
  QDir directory;
  if (not directory.mkpath(directoryPath)) {
    result.error = "Could not create the PDF destination directory";
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
  QTextDocument document;
  document.setHtml(html);
  QPrinter printer(QPrinter::HighResolution);
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setOutputFileName(path);
  printer.setPageMargins(15, 15, 15, 15, QPrinter::Millimeter);
  document.print(&printer);

  QFile file(path);
  if (not file.exists() or not file.open(QIODevice::ReadOnly) or file.size() == 0 or
      not file.read(4).startsWith("%PDF")) {
    file.close();
    QFile::remove(path);
    result.error = "Failed to create a valid PDF file";
    return result;
  }
  file.close();
  result.success = true;
  result.path = path;
  return result;
}
