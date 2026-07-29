#include "html_renderer.hpp"
#include "markdown_renderer.hpp"


QString HtmlRenderer::render(const QList<ChatMessage*>& messages) {
  QString html =
      "<html><head><meta name=\"viewport\" content=\"width=device-width, "
      "initial-scale=1.0\"><style>"
      "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; "
      "color: #E0E0E0; background-color: #1a1a1a; margin: 0; padding: 16px; "
      "line-height: 1.5; }"
      ".message { margin-bottom: 16px; padding: 12px 16px; border-radius: 16px; "
      "word-wrap: break-word; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3); "
      "display: flex; flex-direction: column; max-width: 85%; }"
      ".user { background: linear-gradient(135deg, #0084ff 0%, #0066cc 100%); "
      "color: #ffffff; margin-left: auto; margin-right: 0; border-bottom-right-radius: 4px; "
      "text-align: right; align-items: flex-end; }"
      ".agent { background: linear-gradient(135deg, #2a2a3a 0%, #1f1f2e 100%); "
      "color: #E0E0E0; margin-left: 0; margin-right: auto; border-bottom-left-radius: 4px; "
      "border: 1px solid #404050; }"
      "pre { background-color: #0d0d0d; color: #00ff00; padding: 12px; "
      "border-radius: 8px; border-left: 3px solid #00ff00; overflow-x: auto; "
      "white-space: pre-wrap; font-size: 0.85em; font-family: 'Courier New', monospace; }"
      "code { font-family: 'Courier New', monospace; background-color: #262632; "
      "color: #58d926; padding: 3px 6px; border-radius: 3px; }"
      "a { color: #62baf3; text-decoration: none; } "
      "a:hover { text-decoration: underline; }"
      "p { margin: 8px 0; }"
      "ul, ol { padding-left: 20px; margin: 8px 0; }"
      "h1, h2, h3, h4, h5, h6 { margin: 16px 0 8px 0; font-weight: 600; color: #64b5f6; }"
      "@media print {"
      "body { color: #111; background-color: #fff; padding: 0; }"
      ".message { border: 1px solid #ccc; page-break-inside: avoid; }"
      ".user { background-color: #d1ecf1; color: #000; }"
      ".agent { background-color: #f8f9fa; color: #000; border: 1px solid #dee2e6; }"
      "pre, code { color: #111; background-color: #eee; }"
      "a { color: #0645ad; text-decoration: underline; }"
      "}"
      "</style>"
      // Configure MathJax to typeset the "\( ... \)" / "\[ ... \]" delimiters that
      // markdown_renderer::to_html() rewrites the parsed math spans into.
      "<script>"
      "window.MathJax = {"
      "tex: { inlineMath: [['\\\\(', '\\\\)']], displayMath: [['\\\\[', '\\\\]']] },"
      "options: { skipHtmlTags: ['script', 'noscript', 'style', 'textarea', 'pre', 'code'] }"
      "};"
      "</script>"
      "<script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\"></script>"
      "</head><body>";

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


QString HtmlRenderer::renderMarkdown(const QString& text) {
  if (text.isEmpty())
    return QString();

  return markdown_renderer::to_html(text);
}


QString HtmlRenderer::escapeHtml(const QString& text) {
  return text.toHtmlEscaped();
}
