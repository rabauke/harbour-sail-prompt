#pragma once

#include <QString>


namespace markdown_renderer {

  /// Renders Markdown as an HTML fragment suitable for use as an element's innerHTML.
  /// LaTeX math spans (`$...$` and `$$...$$`) are converted to MathJax delimiters
  /// (`\(...\)` and `\[...\]`) for typesetting by a loaded MathJax renderer.
  /// \param markdown Markdown source text.
  /// \return The rendered HTML fragment.
  QString to_html(const QString& markdown);

}  // namespace markdown_renderer
