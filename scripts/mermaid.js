// Convert ```mermaid fenced code blocks into <div class="mermaid"> so the
// mermaid.js client can render them. Must run before the markdown renderer's
// code highlighting, otherwise the mermaid source gets split into line spans.

function escapeHtml(s) {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

hexo.extend.filter.register('before_post_render', function (data) {
  data.content = data.content.replace(
    /```mermaid\s*\n([\s\S]*?)```/g,
    function (match, code) {
      return '<escape><div class="mermaid">\n' + escapeHtml(code.trim()) + '\n</div></escape>';
    }
  );
  return data;
}, 5); // priority < 10 so this runs before hexo's built-in code highlighter
