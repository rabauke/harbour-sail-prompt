#pragma once

#include <QString>
#include <QList>
#include "chat_message.hpp"


class HtmlRenderer {
public:
  [[nodiscard]] static QString render(const QList<ChatMessage*>& messages);
  [[nodiscard]] static QString render_markdown(const QString& text);
  [[nodiscard]] static QString render_message_block(int index, const ChatMessage* message);

private:
  [[nodiscard]] static QString escape_html(const QString& text);
};
