#include "markdown_renderer.hpp"

#include <QByteArray>
#include <QRegularExpression>

#include <md4c-html.h>


namespace {

void append_output(const MD_CHAR* text, const MD_SIZE size, void* userdata) {
  auto* const output{static_cast<QByteArray*>(userdata)};
  output->append(text, static_cast<int>(size));
}

// md4c's LaTeX math span support only recognizes the standard "$...$" and "$$...$$"
// delimiters. The chat API, however, emits LaTeX math using "\( ... \)" for inline math and
// "\[ ... \]" for display math, so those non-standard delimiters are rewritten to the
// standard ones before the Markdown is parsed.
QString convert_latex_delimiters(const QString& markdown) {
  static const QRegularExpression displayMath{QStringLiteral(R"(\\\[([\s\S]*?)\\\])")};
  static const QRegularExpression inlineMath{QStringLiteral(R"(\\\(([\s\S]*?)\\\))")};

  QString result{markdown};
  result.replace(displayMath, QStringLiteral(R"($$\1$$)"));
  result.replace(inlineMath, QStringLiteral(R"($\1$)"));
  return result;
}

} // namespace


namespace markdown_renderer {

QString to_html(const QString& markdown) {
  // md4c expects UTF-8 input, but Qt strings are UTF-16, so convert first.
  const QByteArray input{convert_latex_delimiters(markdown).toUtf8()};

  QByteArray output;
  constexpr unsigned parser_flags{MD_DIALECT_GITHUB | MD_FLAG_LATEXMATHSPANS};
  constexpr unsigned renderer_flags{0};

  if (md_html(input.constData(), static_cast<MD_SIZE>(input.size()), append_output, &output,
              parser_flags, renderer_flags) != 0) {
    // Parsing failed; fall back to plain, escaped text so nothing is lost.
    return markdown.toHtmlEscaped();
  }

  QString html{QString::fromUtf8(output)};

  // md4c renders LaTeX math spans as <x-equation> elements. Rewrite them to the delimiters
  // expected by MathJax so the webview can typeset the math.
  static const QRegularExpression displayMath{
      QStringLiteral(R"(<x-equation type="display">([\s\S]*?)</x-equation>)")};
  static const QRegularExpression inlineMath{
      QStringLiteral(R"(<x-equation>([\s\S]*?)</x-equation>)")};

  html.replace(displayMath, QStringLiteral(R"(\[\1\])"));
  html.replace(inlineMath, QStringLiteral(R"(\(\1\))"));

  return html;
}

} // namespace markdown_renderer
