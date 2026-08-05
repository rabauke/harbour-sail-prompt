#include <QCoreApplication>
#include <QJsonArray>
#include <iostream>
#include "open_ai_api.hpp"
#include "html_renderer.hpp"
#include "chat_message_list_model.hpp"

namespace {
  bool require(bool condition, const char *message) {
    if (!condition)
      std::cerr << "FAILED: " << message << std::endl;
    return condition;
  }
}  // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  ChatMessageListModel messageModel;
  int countChanges = 0;
  QObject::connect(&messageModel, &ChatMessageListModel::countChanged,
                   [&countChanges]() { ++countChanges; });
  messageModel.add(new ChatMessage(ChatMessage::User, "test"));
  if (!require(messageModel.count() == 1 and countChanges == 1, "message count after add"))
    return 1;
  messageModel.remove(0);
  if (!require(messageModel.count() == 0 and countChanges == 2, "message count after remove"))
    return 1;
  messageModel.add(new ChatMessage(ChatMessage::User, "one"));
  messageModel.add(new ChatMessage(ChatMessage::Agent, "two"));
  messageModel.clear();
  if (!require(messageModel.count() == 0 and countChanges == 5, "message count after clear"))
    return 1;

  QList<open_ai_api::Message> messages;
  open_ai_api::Message system = {"system", "Be concise"};
  open_ai_api::Message user = {"user", "Hello"};
  messages << system << user;
  const QJsonObject payload = open_ai_api::createChatPayload(messages, "test-model");
  if (!require(payload.value("model").toString() == "test-model", "payload model") ||
      !require(payload.value("stream").toBool(), "payload streaming flag") ||
      !require(payload.value("messages").toArray().size() == 2, "payload messages") ||
      !require(
          payload.value("messages").toArray().at(1).toObject().value("content").toString() ==
              "Hello",
          "payload content"))
    return 1;

  open_ai_api::SseParser parser;
  QStringList chunks;
  open_ai_api::Error error;
  error = parser.append("data: {\"choices\":[{\"delta\":{\"content\":\"Hel", false, &chunks);
  error = parser.append("lo\"}}]}\r\n\r\ndata: {\"choices\":[{\"delta\":\r\n", false, &chunks);
  error = parser.append("data: {\"content\":\" world\"}}]}\r\n\r\ndata: [DO", false, &chunks);
  error = parser.append("NE]", true, &chunks);
  if (!require(error.error_code == open_ai_api::Error::none, "fragmented SSE error") ||
      !require(chunks.join("") == "Hello world", "fragmented SSE content") ||
      !require(parser.done(), "unterminated DONE marker"))
    return 1;

  open_ai_api::SseParser malformed;
  QStringList ignored;
  error = malformed.append("data: {not-json}\n\n", false, &ignored);
  if (!require(error.error_code == open_ai_api::Error::invalid_response,
               "malformed SSE rejection"))
    return 1;

  open_ai_api::SseParser apiError;
  error =
      apiError.append("data: {\"error\":{\"message\":\"unknown model\"}}\n\n", false, &ignored);
  if (!require(error.error_code == open_ai_api::Error::invalid_model, "stream API error"))
    return 1;

  ChatMessage systemMessage(ChatMessage::System, "<secret>");
  ChatMessage userMessage(ChatMessage::User,
                          "<tag> & text\nsecond $x^2$\n\n"
                          "[safe](https://example.com/?a=1&b=2) [bad](javascript:alert(1))\n\n"
                          "1. first\n2. second\n- bullet\n\n`<code>`\n```\n<script>\n");
  ChatMessage agentMessage(ChatMessage::Agent, "reply");
  QList<ChatMessage *> renderedMessages;
  renderedMessages << &systemMessage << &userMessage << &agentMessage;
  const QString html = HtmlRenderer::render(renderedMessages);

  if (!require(!html.contains("id=\"msg-0\""), "system message gets no id") ||
      !require(html.contains("id=\"msg-1\" class=\"message user\""), "user message id") ||
      !require(html.contains("id=\"msg-2\" class=\"message agent\">" +
                              HtmlRenderer::render_markdown(agentMessage.content()) + "</div>"),
               "agent message id"))
    return 1;

  if (!require(HtmlRenderer::render_message_block(0, &systemMessage).isEmpty(),
               "renderMessageBlock excludes system messages") ||
      !require(HtmlRenderer::render_message_block(1, &userMessage) ==
                   "<div id=\"msg-1\" class=\"message user\">" +
                       HtmlRenderer::render_markdown(userMessage.content()) + "</div>",
               "renderMessageBlock matches render() fragment for user message") ||
      !require(HtmlRenderer::render_message_block(2, &agentMessage) ==
                   "<div id=\"msg-2\" class=\"message agent\">" +
                       HtmlRenderer::render_markdown(agentMessage.content()) + "</div>",
               "renderMessageBlock matches render() fragment for agent message"))
    return 1;

  ChatMessage emptyAgentMessage(ChatMessage::Agent, "");
  if (!require(HtmlRenderer::render_message_block(3, &emptyAgentMessage) ==
                   "<div id=\"msg-3\" class=\"message agent\"></div>",
               "renderMessageBlock handles empty assistant placeholder"))
    return 1;

  if (!require(!html.contains("secret"), "system message filtering") ||
      !require(html.contains("&lt;tag&gt; &amp; text<br>second $x^2$"),
               "escaping and multiline paragraph") ||
      !require(html.contains("href=\"https://example.com/?a=1&amp;b=2\""), "safe link") ||
      !require(!html.contains("href=\"javascript:"), "malicious link rejection") ||
      !require(html.contains("<ol><li>first</li><li>second</li></ol><ul><li>bullet</li></ul>"),
               "ordered and unordered lists") ||
      !require(html.contains("<code>&lt;code&gt;</code>"), "inline code escaping") ||
      !require(html.contains("<pre><code>&lt;script&gt;\n") and html.contains("</code></pre>"),
               "unclosed code block") ||
      !require(!html.contains("<script>window"), "no executable scrolling script"))
    return 1;

  std::cout << "All payload, SSE, and HTML renderer tests passed" << std::endl;
  return 0;
}
