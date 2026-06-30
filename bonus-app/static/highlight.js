// Resaltado de sintaxis (sin dependencias): editor del lenguaje propio y ASM.
(function () {
  const KEYWORDS = new Set([
    "fun", "endfun", "return", "if", "then", "else", "endif",
    "while", "do", "endwhile", "dowhile", "enddo", "break",
    "switch", "case", "default", "endswitch", "var", "struct",
    "new", "true", "false", "print", "sqrt", "lambda", "endlambda",
    "int", "float", "string", "list", "matrix",
  ]);

  function esc(s) {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  // Resaltado del código fuente del lenguaje.
  window.highlightCode = function (src) {
    // patrón global: string | float | número | identificador | operador | otro
    const re = /("(?:[^"\\]|\\.)*")|(\d+\.\d+)|(\d+)|([A-Za-z_]\w*)|([+\-*/=<>!&|.,;()\[\]{}]+)|(\s+)|(.)/g;
    let out = "", m;
    while ((m = re.exec(src)) !== null) {
      if (m[1]) out += `<span class="tok-string">${esc(m[1])}</span>`;
      else if (m[2]) out += `<span class="tok-num">${esc(m[2])}</span>`;
      else if (m[3]) out += `<span class="tok-num">${esc(m[3])}</span>`;
      else if (m[4]) out += KEYWORDS.has(m[4])
        ? `<span class="tok-keyword">${m[4]}</span>` : esc(m[4]);
      else if (m[5]) out += `<span class="tok-op">${esc(m[5])}</span>`;
      else out += esc(m[0]);
    }
    return out;
  };

  // Resaltado del ensamblador AT&T.
  window.highlightAsm = function (src) {
    return src.split("\n").map((line) => {
      const t = line.trim();
      if (t.startsWith("#")) return `<span class="asm-comment">${esc(line)}</span>`;
      if (t.startsWith(".")) {
        // directiva (.data, .text, .globl, .string, ...)
        return line.replace(/^(\s*)(\.[A-Za-z0-9_.]+)/,
          (_, sp, d) => `${sp}<span class="asm-dir">${d}</span>`);
      }
      // etiqueta:  algo:
      if (/^[A-Za-z_.][\w.$]*:/.test(t)) {
        return line.replace(/^(\s*)([\w.$]+:)/,
          (_, sp, l) => `${sp}<span class="asm-label">${l}</span>`);
      }
      // instrucción:  mnem ops
      let r = esc(line);
      r = r.replace(/^(\s*)([a-z][a-z0-9]+)/, (_, sp, mn) => `${sp}<span class="asm-mnem">${mn}</span>`);
      r = r.replace(/(%[a-z0-9]+)/g, '<span class="asm-reg">$1</span>');
      r = r.replace(/(\$[-]?\w+)/g, '<span class="asm-imm">$1</span>');
      return r;
    }).join("\n");
  };
})();
