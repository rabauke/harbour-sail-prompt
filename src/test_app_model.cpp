#include <QCoreApplication>
#include <QJsonArray>
#include <iostream>
#include "open_ai_api.hpp"
#include "html_renderer.hpp"
#include "chat_message_list_model.hpp"

namespace {
  bool require(bool condition, const char* message) {
    if (!condition)
      std::cerr << "FAILED: " << message << std::endl;
    return condition;
  }
}  // namespace

int main(int argc, char* argv[]) {
  QCoreApplication app{argc, argv};

  ChatMessageListModel messageModel;
  int countChanges{0};
  QObject::connect(&messageModel, &ChatMessageListModel::countChanged,
                   [&countChanges]() { ++countChanges; });
  messageModel.add(new ChatMessage{ChatMessage::User, "test"});
  if (!require(messageModel.count() == 1 and countChanges == 1, "message count after add"))
    return 1;
  messageModel.remove(0);
  if (!require(messageModel.count() == 0 and countChanges == 2, "message count after remove"))
    return 1;
  messageModel.add(new ChatMessage{ChatMessage::User, "one"});
  messageModel.add(new ChatMessage{ChatMessage::Agent, "two"});
  messageModel.clear();
  if (!require(messageModel.count() == 0 and countChanges == 5, "message count after clear"))
    return 1;

  QList<open_ai_api::Message> messages;
  open_ai_api::Message system{"system", "Be concise"};
  open_ai_api::Message user{"user", "Hello"};
  messages << system << user;
  const QJsonObject payload{open_ai_api::createChatPayload(messages, "test-model")};
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

  ChatMessage systemMessage{ChatMessage::System, "<secret>"};
  ChatMessage userMessage{ChatMessage::User,
                          "<tag> & text\nsecond $x^2$\n\n"
                          "[safe](https://example.com/?a=1&b=2) [bad](javascript:alert(1))\n\n"
                          "1. first\n2. second\n- bullet\n\n`<code>`\n```\n<script>\n"};
  ChatMessage agentMessage{ChatMessage::Agent, "reply"};
  QList<ChatMessage*> renderedMessages;
  renderedMessages << &systemMessage << &userMessage << &agentMessage;
  const QString html{HtmlRenderer::render(renderedMessages)};
  const QString largerFontHtml{HtmlRenderer::render(renderedMessages, 24)};

  if (!require(!html.contains("id=\"msg-0\""), "system message gets no id") ||
      !require(html.contains("font-size: 16px;"), "default web view font size") ||
      !require(largerFontHtml.contains("font-size: 24px;"), "configured web view font size") ||
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

  ChatMessage emptyAgentMessage{ChatMessage::Agent, ""};
  if (!require(HtmlRenderer::render_message_block(3, &emptyAgentMessage) ==
                   "<div id=\"msg-3\" class=\"message agent\"></div>",
               "renderMessageBlock handles empty assistant placeholder"))
    return 1;

  if (!require(!html.contains("secret"), "system message filtering") ||
      !require(html.contains("&lt;tag&gt; &amp; text\nsecond \\(x^2\\)"),
               "escaping and multiline paragraph") ||
      !require(html.contains("href=\"https://example.com/?a=1&amp;b=2\""), "safe link") ||
      !require(!html.contains("href=\"javascript:"), "malicious link rejection") ||
      !require(html.contains("<ol>\n<li>first</li>\n<li>second</li>\n</ol>\n<ul>\n<li>bullet</li>\n</ul>"),
               "ordered and unordered lists") ||
      !require(html.contains("<code>&lt;code&gt;</code>"), "inline code escaping") ||
      !require(html.contains("<pre><code>&lt;script&gt;\n") and html.contains("</code></pre>"),
               "unclosed code block") ||
      !require(!html.contains("<script>window"), "no executable scrolling script"))
    return 1;

  // Security test suite: HTML & JavaScript injection vector mitigation
  const QString xssScript{HtmlRenderer::render_markdown("<script>alert('xss')</script>")};
  if (!require(!xssScript.contains("<script>"), "script tag must not render raw") ||
      !require(xssScript.contains("&lt;script&gt;"), "script tag must be escaped"))
    return 1;

  const QString xssImg{HtmlRenderer::render_markdown("<img src=\"x\" onerror=\"alert(1)\">")};
  if (!require(!xssImg.contains("<img src=\"x\""), "img tag with onerror must not render raw") ||
      !require(xssImg.contains("&lt;img"), "img tag must be escaped"))
    return 1;

  const QString xssSvg{HtmlRenderer::render_markdown("<svg onload=\"alert(1)\">")};
  if (!require(!xssSvg.contains("<svg"), "svg tag must not render raw") ||
      !require(xssSvg.contains("&lt;svg"), "svg tag must be escaped"))
    return 1;

  const QString xssIframe{HtmlRenderer::render_markdown("<iframe src=\"https://evil.com\"></iframe>")};
  if (!require(!xssIframe.contains("<iframe"), "iframe tag must not render raw") ||
      !require(xssIframe.contains("&lt;iframe"), "iframe tag must be escaped"))
    return 1;

  const QString xssComment{HtmlRenderer::render_markdown("<!-- <script>alert(1)</script> -->")};
  if (!require(!xssComment.contains("<!--"), "HTML comment must not render raw") ||
      !require(xssComment.contains("&lt;!--"), "HTML comment must be escaped"))
    return 1;

  const QString xssDiv{HtmlRenderer::render_markdown("<div onclick=\"evil()\">click</div>")};
  if (!require(!xssDiv.contains("<div onclick"), "div tag with handler must not render raw") ||
      !require(xssDiv.contains("&lt;div"), "div tag must be escaped"))
    return 1;

  // Security test suite: Dangerous link scheme neutralization
  const QString jsLink{HtmlRenderer::render_markdown("[js link](javascript:alert(1))")};
  if (!require(!jsLink.contains("href=\"javascript:"), "javascript link scheme must be stripped") ||
      !require(jsLink.contains("href=\"#\""), "unsafe javascript link must be neutralized to '#'"))
    return 1;

  const QString dataLink{HtmlRenderer::render_markdown("[data link](data:text/html,<script>alert(1)</script>)")};
  if (!require(!dataLink.contains("href=\"data:"), "data link scheme must be stripped") ||
      !require(dataLink.contains("href=\"#\""), "unsafe data link must be neutralized to '#'"))
    return 1;

  const QString vbsLink{HtmlRenderer::render_markdown("[vbs link](vbscript:msgbox(1))")};
  if (!require(!vbsLink.contains("href=\"vbscript:"), "vbscript link scheme must be stripped") ||
      !require(vbsLink.contains("href=\"#\""), "unsafe vbscript link must be neutralized to '#'"))
    return 1;

  const QString encodedJsLink{HtmlRenderer::render_markdown("[encoded js](javascript&#x3a;alert(1))")};
  if (!require(!encodedJsLink.contains("javascript"), "encoded javascript link must be neutralized") ||
      !require(encodedJsLink.contains("href=\"#\""), "encoded javascript link must be neutralized to '#'"))
    return 1;

  // Markdown rendering fidelity test suite
  const QString headings{HtmlRenderer::render_markdown("# Heading 1\n## Heading 2\n### Heading 3")};
  if (!require(headings.contains("<h1>Heading 1</h1>"), "h1 heading formatting") ||
      !require(headings.contains("<h2>Heading 2</h2>"), "h2 heading formatting") ||
      !require(headings.contains("<h3>Heading 3</h3>"), "h3 heading formatting"))
    return 1;

  const QString formatting{HtmlRenderer::render_markdown("**bold** and *italic* and ~~strike~~")};
  if (!require(formatting.contains("<strong>bold</strong>"), "bold formatting") ||
      !require(formatting.contains("<em>italic</em>"), "italic formatting") ||
      !require(formatting.contains("<del>strike</del>"), "strikethrough formatting"))
    return 1;

  const QString tableMd{"| Col 1 | Col 2 |\n| --- | --- |\n| Val 1 | Val 2 |"};
  const QString tableHtml{HtmlRenderer::render_markdown(tableMd)};
  if (!require(tableHtml.contains("<table>") && tableHtml.contains("<th>Col 1</th>") &&
                   tableHtml.contains("<td>Val 1</td>"),
               "table formatting"))
    return 1;

  const QString blockquote{HtmlRenderer::render_markdown("> quoted message")};
  if (!require(blockquote.contains("<blockquote>") && blockquote.contains("<p>quoted message</p>"),
               "blockquote formatting"))
    return 1;

  const QString fencedCode{HtmlRenderer::render_markdown("```cpp\n#include <iostream>\n```")};
  if (!require(fencedCode.contains("<pre><code class=\"language-cpp\">#include &lt;iostream&gt;\n</code></pre>"),
               "fenced code block with angle bracket escaping"))
    return 1;

  // LaTeX Math preservation test suite
  const QString mathMd{"Inline $E=mc^2$ and \\(a^2+b^2=c^2\\)\n\nBlock:\n$$x = \\frac{-b}{2a}$$\n\n\\[y = mx + b\\]"};
  const QString mathHtml{HtmlRenderer::render_markdown(mathMd)};
  if (!require(mathHtml.contains("\\(E=mc^2\\)"), "inline dollar math translation") ||
      !require(mathHtml.contains("\\(a^2+b^2=c^2\\)"), "inline backslash-paren math translation") ||
      !require(mathHtml.contains("\\[x = \\frac{-b}{2a}\\]"), "display dollar math translation") ||
      !require(mathHtml.contains("\\[y = mx + b\\]"), "display backslash-bracket math translation"))
    return 1;

  const QString spoofedMath{HtmlRenderer::render_markdown("<x-equation type=\"display\">evil</x-equation>")};
  if (!require(!spoofedMath.contains("\\[evil\\]"), "spoofed x-equation must not convert to MathJax") ||
      !require(spoofedMath.contains("&lt;x-equation"), "spoofed x-equation must be escaped as text"))
    return 1;

  std::cout << "All payload, SSE, and HTML renderer tests passed" << std::endl;
  return 0;
}
