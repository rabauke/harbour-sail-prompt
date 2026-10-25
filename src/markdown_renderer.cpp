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


  QString decode_html_entities(const QString& input) {
    QString result;
    result.reserve(input.size());
    for (int i{0}; i < input.size(); ++i) {
      if (input.at(i) == QLatin1Char('&')) {
        const int semi{input.indexOf(QLatin1Char(';'), i + 1)};
        if (semi != -1 and semi - i <= 10) {
          const QString entity{input.mid(i + 1, semi - i - 1)};
          if (entity.startsWith(QLatin1Char('#'))) {
            bool ok{false};
            int codePoint{0};
            if (entity.startsWith(QLatin1String("#x")) or entity.startsWith(QLatin1String("#X"))) {
              codePoint = entity.mid(2).toInt(&ok, 16);
            } else {
              codePoint = entity.mid(1).toInt(&ok, 10);
            }
            if (ok and codePoint > 0 and codePoint <= 0x10FFFF) {
              result.append(QChar(codePoint));
              i = semi;
              continue;
            }
          } else {
            const QString lower{entity.toLower()};
            if (lower == QLatin1String("amp")) {
              result.append(QLatin1Char('&'));
              i = semi;
              continue;
            } else if (lower == QLatin1String("colon")) {
              result.append(QLatin1Char(':'));
              i = semi;
              continue;
            } else if (lower == QLatin1String("tab") or lower == QLatin1String("newline")) {
              result.append(QLatin1Char(' '));
              i = semi;
              continue;
            }
          }
        }
      }
      result.append(input.at(i));
    }
    return result;
  }

  bool is_safe_url(const QString& rawUrl) {
    const QString decoded{decode_html_entities(rawUrl)};
    QString cleaned;
    cleaned.reserve(decoded.size());
    for (int i{0}; i < decoded.size(); ++i) {
      const QChar ch{decoded.at(i)};
      if (!ch.isSpace() and ch.toLatin1() >= 32 and ch.toLatin1() != 127) {
        cleaned.append(ch);
      }
    }

    if (cleaned.startsWith(QLatin1Char('#')) or cleaned.startsWith(QLatin1Char('/'))) {
      return true;
    }

    const int colon{cleaned.indexOf(QLatin1Char(':'))};
    if (colon == -1) {
      return true;
    }

    const QString scheme{cleaned.left(colon).toLower()};
    if (scheme == QLatin1String("http") or scheme == QLatin1String("https") or
        scheme == QLatin1String("mailto")) {
      return true;
    }

    return false;
  }

  QString sanitize_links(const QString& html) {
    static const QRegularExpression linkRegex{QStringLiteral(R"raw((href|src)="([^"]*)")raw")};
    QString result;
    int lastPos{0};
    auto matchIterator{linkRegex.globalMatch(html)};
    while (matchIterator.hasNext()) {
      const auto match{matchIterator.next()};
      result.append(html.mid(lastPos, match.capturedStart() - lastPos));
      const QString attrName{match.captured(1)};
      const QString url{match.captured(2)};
      if (is_safe_url(url)) {
        result.append(match.captured(0));
      } else {
        result.append(attrName + QStringLiteral("=\"#\""));
      }
      lastPos = match.capturedEnd();
    }
    result.append(html.mid(lastPos));
    return result;
  }

}  // namespace


namespace markdown_renderer {

  QString to_html(const QString& markdown) {
    // md4c expects UTF-8 input, but Qt strings are UTF-16, so convert first.
    const QByteArray input{convert_latex_delimiters(markdown).toUtf8()};

    QByteArray output;
    constexpr unsigned parser_flags{MD_DIALECT_GITHUB | MD_FLAG_LATEXMATHSPANS | MD_FLAG_NOHTML};
    constexpr unsigned renderer_flags{0};

    if (md_html(input.constData(), static_cast<MD_SIZE>(input.size()), append_output, &output,
                parser_flags, renderer_flags) != 0) {
      // Parsing failed; fall back to plain, escaped text so nothing is lost.
      return markdown.toHtmlEscaped();
    }

    QString html{QString::fromUtf8(output)};
    html = sanitize_links(html);

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

}  // namespace markdown_renderer
