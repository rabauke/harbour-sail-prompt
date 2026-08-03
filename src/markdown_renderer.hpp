#pragma once

#include <QString>


namespace markdown_renderer {

// Renders the given Markdown text as an HTML fragment suitable for use as innerHTML of an
// element. LaTeX math spans (`$...$` and `$$...$$`) are converted to MathJax delimiters
// (`\(...\)` and `\[...\]`) so that a MathJax renderer loaded on the page can typeset them.
QString to_html(const QString& markdown);

} // namespace markdown_renderer
