#pragma once

#include <QString>
#include <QList>
#include "chat_message.hpp"


class HtmlRenderer {
public:
  static QString render(const QList<ChatMessage*>& messages);
  static QString renderMarkdown(const QString& text);
  static QString renderMessageBlock(int index, ChatMessage* message);

private:
  static QString escapeHtml(const QString& text);
};
