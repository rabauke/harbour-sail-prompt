#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <iostream>
#include "AiChatModel/chat_message.hpp"
#include "AiChatModel/pdf_exporter.hpp"

namespace {
  bool require(bool condition, const char *message) {
    if (!condition)
      std::cerr << "FAILED: " << message << std::endl;
    return condition;
  }
}  // namespace

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  std::cout << "Testing PDF export..." << std::endl;

  if (!require(PdfExporter::sanitizeTitle("  A / title:*?  ") == "A_title",
               "filename sanitization"))
    return 1;
  if (!require(PdfExporter::sanitizeTitle("/.:*") == "chat", "empty sanitized title fallback"))
    return 1;

  QTemporaryDir temporaryDirectory;
  if (!require(temporaryDirectory.isValid(), "temporary directory creation"))
    return 1;
  const QDateTime timestamp(QDate(2026, 7, 26), QTime(22, 23, 0));
  const QString firstPath =
      PdfExporter::outputPath(temporaryDirectory.path(), "A title", timestamp);
  QFile collision(firstPath);
  if (!require(collision.open(QIODevice::WriteOnly), "collision fixture creation"))
    return 1;
  collision.close();
  const QString secondPath =
      PdfExporter::outputPath(temporaryDirectory.path(), "A title", timestamp);
  if (!require(secondPath.endsWith("A_title_20260726_222300_2.pdf"), "collision-safe path"))
    return 1;

  QList<ChatMessage *> messages;
  messages.append(new ChatMessage(ChatMessage::Agent, "Agent only"));
  PdfExporter::Result result = PdfExporter::exportMessages(messages, temporaryDirectory.path());
  if (!require(!result.success and result.error.contains("user message"),
               "user message required"))
    return 1;
  qDeleteAll(messages);
  messages.clear();

  messages.append(new ChatMessage(ChatMessage::User, "# Héllo PDF\n\n* Unicode: €, 中文, 🚀"));
  messages.append(new ChatMessage(ChatMessage::Agent, "**Rendered Markdown**"));
  messages.append(new ChatMessage(ChatMessage::System, "API-SECRET-SYSTEM-PROMPT"));
  QString htmlError;
  const QString html = PdfExporter::htmlForMessages(messages, &htmlError);
  if (!require(htmlError.isEmpty() and html.contains("Rendered Markdown") &&
                   !html.contains("API-SECRET-SYSTEM-PROMPT"),
               "HTML source secret exclusion"))
    return 1;
  if (!require(html.contains("@media print") and html.contains("background-color: #fff"),
               "print styling"))
    return 1;

  result = PdfExporter::exportMessages(messages, temporaryDirectory.path());
  if (!require(result.success, "Unicode Markdown PDF export")) {
    std::cerr << "Export error: " << result.error.toStdString() << std::endl;
    return 1;
  }
  QFile pdf(result.path);
  if (!require(pdf.open(QIODevice::ReadOnly) and pdf.size() > 0 and pdf.read(4) == "%PDF",
               "valid PDF output"))
    return 1;
  pdf.close();

  const PdfExporter::Result repeated =
      PdfExporter::exportMessages(messages, temporaryDirectory.path());
  if (!require(repeated.success and repeated.path != result.path,
               "repeated export has distinct path"))
    return 1;

  const QString fileDestination = temporaryDirectory.path() + "/not-a-directory";
  QFile destination(fileDestination);
  if (!require(destination.open(QIODevice::WriteOnly), "non-directory fixture creation"))
    return 1;
  destination.close();
  const PdfExporter::Result invalidDestination =
      PdfExporter::exportMessages(messages, fileDestination);
  if (!require(
          !invalidDestination.success and invalidDestination.error.contains("not a directory"),
          "non-directory destination failure"))
    return 1;

  qDeleteAll(messages);

  std::cout << "All PDF export tests passed" << std::endl;
  return 0;
}
