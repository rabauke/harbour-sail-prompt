#include "html_renderer.hpp"
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>


QString HtmlRenderer::escapeHtml(const QString& text) {
  return text.toHtmlEscaped();
}


QString HtmlRenderer::render(const QList<ChatMessage*>& messages) {
  QString html =
      "<html><head><meta name=\"viewport\" content=\"width=device-width, "
      "initial-scale=1.0\"><style>"
      "body { font-family: sans-serif; color: #E6E6E6; background-color: transparent; margin: "
      "0; padding: 10px; line-height: 1.4; }"
      ".message { margin-bottom: 24px; padding: 12px; border-radius: 12px; word-wrap: "
      "break-word; }"
      ".user { background-color: rgba(255, 255, 255, 0.15); margin-left: 20px; "
      "border-bottom-right-radius: 4px; }"
      ".agent { background-color: rgba(255, 255, 255, 0.08); margin-right: 20px; "
      "border-bottom-left-radius: 4px; }"
      "pre { background-color: rgba(0, 0, 0, 0.4); padding: 12px; border-radius: 6px; "
      "overflow-x: auto; white-space: pre-wrap; font-size: 0.9em; }"
      "code { font-family: monospace; background-color: rgba(0, 0, 0, 0.3); padding: 2px 4px; "
      "border-radius: 3px; }"
      "a { color: #62baf3; text-decoration: none; }"
      "p { margin: 8px 0; }"
      "ul, ol { padding-left: 20px; }"
      "h1, h2, h3 { margin: 16px 0 8px 0; }"
      "@media print {"
      "body { color: #111; background-color: #fff; padding: 0; }"
      ".message { border: 1px solid #ccc; page-break-inside: avoid; }"
      ".user { background-color: #f2f2f2; }"
      ".agent { background-color: #fff; }"
      "pre, code { color: #111; background-color: #eee; }"
      "a { color: #0645ad; text-decoration: underline; }"
      "}"
      "</style></head><body>";

  for (int i = 0; i < messages.size(); ++i) {
    ChatMessage* msg = messages.at(i);
    if (msg->role() == ChatMessage::System)
      continue;

    QString roleClass = (msg->role() == ChatMessage::User) ? "user" : "agent";
    html += QString("<div class=\"message %1\">").arg(roleClass);
    html += renderMarkdown(msg->content());
    html += "</div>";
  }

  html += "<div id=\"end\"></div>";
  html += "</body></html>";
  return html;
}


static QString formatPlain(QString text) {
  text = text.toHtmlEscaped();
  text.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<b>\\1</b>");
  text.replace(QRegularExpression("__([^_]+)__"), "<b>\\1</b>");
  text.replace(QRegularExpression("\\*([^*]+)\\*"), "<i>\\1</i>");
  text.replace(QRegularExpression("_([^_]+)_"), "<i>\\1</i>");
  return text;
}


static bool safeLink(const QString& value) {
  const QUrl url(value.trimmed());
  const QString scheme = url.scheme().toLower();
  return url.isValid() && (scheme == "http" || scheme == "https" || scheme == "mailto");
}


static QString processInline(const QString& text) {
  const QRegularExpression token("`([^`]*)`|\\[([^\\]]+)\\]\\(([^\\s)]+)\\)");
  QRegularExpressionMatchIterator matches = token.globalMatch(text);
  QString result;
  int position = 0;
  while (matches.hasNext()) {
    const QRegularExpressionMatch match = matches.next();
    result += formatPlain(text.mid(position, match.capturedStart() - position));
    if (!match.captured(1).isNull()) {
      result += "<code>" + match.captured(1).toHtmlEscaped() + "</code>";
    } else if (safeLink(match.captured(3))) {
      result += "<a href=\"" + match.captured(3).trimmed().toHtmlEscaped() + "\">" +
                formatPlain(match.captured(2)) + "</a>";
    } else {
      result += formatPlain(match.captured(0));
    }
    position = match.capturedEnd();
  }
  result += formatPlain(text.mid(position));
  return result;
}


QString HtmlRenderer::renderMarkdown(const QString& text) {
  if (text.isEmpty())
    return QString();

  QStringList lines = text.split('\n');
  QString html;
  bool inCodeBlock = false;
  enum ListType { NoList, UnorderedList, OrderedList };
  ListType listType = NoList;
  QString codeBlockContent;
  QStringList paragraph;

  const auto flushParagraph = [&html, &paragraph]() {
    if (!paragraph.isEmpty()) {
      html += "<p>" + processInline(paragraph.join("\n")).replace("\n", "<br>") + "</p>";
      paragraph.clear();
    }
  };
  const auto closeList = [&html, &listType]() {
    if (listType != NoList) {
      html += listType == OrderedList ? "</ol>" : "</ul>";
      listType = NoList;
    }
  };

  for (int i = 0; i < lines.size(); ++i) {
    QString line = lines[i];

    // Code blocks
    if (line.trimmed().startsWith("```")) {
      flushParagraph();
      closeList();
      if (inCodeBlock) {
        html += "<pre><code>" + escapeHtml(codeBlockContent) + "</code></pre>";
        codeBlockContent.clear();
        inCodeBlock = false;
      } else {
        inCodeBlock = true;
      }
      continue;
    }

    if (inCodeBlock) {
      codeBlockContent += line + "\n";
      continue;
    }

    // Headings
    if (line.startsWith("#")) {
      int level = 0;
      while (level < line.length() && line[level] == '#')
        level++;
      if (level > 0 && level <= 6 && level < line.length() && line[level] == ' ') {
        QString content = line.mid(level + 1).trimmed();
        flushParagraph();
        closeList();
        html +=
            QString("<h%1>").arg(level) + processInline(content) + QString("</h%1>").arg(level);
        continue;
      }
    }

    // Lists
    const bool ordered = QRegularExpression("^\\s*\\d+\\.\\s").match(line).hasMatch();
    const bool unordered = line.trimmed().startsWith("* ") || line.trimmed().startsWith("- ");
    if (ordered || unordered) {
      flushParagraph();
      const ListType wanted = ordered ? OrderedList : UnorderedList;
      if (listType != wanted) {
        closeList();
        html += wanted == OrderedList ? "<ol>" : "<ul>";
        listType = wanted;
      }
      QString content = line.trimmed();
      int spaceIdx = content.indexOf(' ');
      if (spaceIdx != -1) {
        content = content.mid(spaceIdx + 1);
      }
      html += "<li>" + processInline(content) + "</li>";
      continue;
    } else {
      closeList();
    }

    // Paragraphs
    if (line.trimmed().isEmpty()) {
      flushParagraph();
      continue;
    }
    paragraph.append(line);
  }

  if (inCodeBlock) {
    html += "<pre><code>" + escapeHtml(codeBlockContent) + "</code></pre>";
  }
  flushParagraph();
  closeList();

  return html;
}
