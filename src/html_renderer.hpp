#pragma once

#include <QString>
#include <QList>
#include "chat_message.hpp"


/// Converts conversation messages and Markdown into HTML for display.
class HtmlRenderer {
public:
  /// Renders a complete HTML document containing the supplied conversation messages.
  /// System messages are omitted and the font size is clamped to the supported range.
  /// \param messages Messages to render.
  /// \param fontSize Requested body font size in pixels.
  /// \return The complete HTML document.
  [[nodiscard]] static QString render(const QList<ChatMessage*>& messages, int fontSize = 16);

  /// Converts Markdown text into an HTML fragment.
  /// \param text Markdown source text.
  /// \return The rendered HTML fragment, or an empty string for empty input.
  [[nodiscard]] static QString render_markdown(const QString& text);

  /// Renders one non-system message as an HTML block.
  /// \param index Message index used to identify the block.
  /// \param message Message to render.
  /// \return The message's HTML block, or an empty string for a system message.
  [[nodiscard]] static QString render_message_block(int index, const ChatMessage* message);

private:
  [[nodiscard]] static QString escape_html(const QString& text);
};
