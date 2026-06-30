// ════════ CompiLab — lógica de la aplicación ════════
const $ = (id) => document.getElementById(id);
const code = $("code"), highlight = $("highlight"), gutter = $("gutter"),
      editorScroll = $("editorScroll");
let lastAsm = "";

// ── Editor: resaltado + numeración + sincronización de scroll ──
function refreshEditor() {
  const src = code.value;
  highlight.innerHTML = window.highlightCode(src) + "\n";
  const lines = src.split("\n").length;
  let g = "";
  for (let i = 1; i <= lines; i++) g += i + "\n";
  gutter.textContent = g;
  code.style.height = "auto";
  code.style.height = Math.max(code.scrollHeight, editorScroll.clientHeight) + "px";
}
code.addEventListener("input", refreshEditor);
code.addEventListener("keydown", (e) => {
  if (e.key === "Tab") { // insertar 4 espacios
    e.preventDefault();
    const s = code.selectionStart, en = code.selectionEnd;
    code.value = code.value.slice(0, s) + "    " + code.value.slice(en);
    code.selectionStart = code.selectionEnd = s + 4;
    refreshEditor();
  }
});
editorScroll.addEventListener("scroll", () => {
  gutter.style.transform = `translateY(${-editorScroll.scrollTop}px)`;
});

// ── Tabs ──
document.querySelectorAll(".tab").forEach((t) => {
  t.onclick = () => {
    document.querySelectorAll(".tab").forEach((x) => x.classList.remove("active"));
    document.querySelectorAll(".view").forEach((x) => x.classList.remove("active"));
    t.classList.add("active");
    document.querySelector(`.view[data-view="${t.dataset.tab}"]`).classList.add("active");
    if (t.dataset.tab === "ast") window.AST.fit();
  };
});

// ── Pipeline (breadcrumb de fases) ──
const STAGES = [
  { id: "lexico", label: "Léxico", ic: "❑" },
  { id: "sintactico", label: "Sintáctico", ic: "⌬" },
  { id: "semantico", label: "Semántico", ic: "✓" },
  { id: "generacion", label: "Generación x86", ic: "⚙" },
  { id: "ejecucion", label: "Ejecución", ic: "▸" },
];
function renderPipeline(stages) {
  const el = $("pipeline");
  el.innerHTML = "";
  STAGES.forEach((s, i) => {
    const st = (stages && stages[s.id]) || "";
    const span = document.createElement("span");
    span.className = "stage" + (st ? " " + st : "");
    span.innerHTML = `<span class="ic">${s.ic}</span> ${s.label}`;
    el.appendChild(span);
    if (i < STAGES.length - 1) {
      const sep = document.createElement("span");
      sep.className = "stage-sep"; sep.textContent = "→";
      el.appendChild(sep);
    }
  });
}
renderPipeline({});

// ── Tokens ──
const TOK_KW = new Set(["FUN","ENDFUN","RETURN","IF","THEN","ELSE","ENDIF","WHILE","DO",
  "ENDWHILE","DOWHILE","ENDDO","BREAK","SWITCH","CASE","DEFAULT","ENDSWITCH","VAR","STRUCT",
  "NEW","TRUE","FALSE","PRINT","SQRT","LAMBDA","ENDLAMBDA"]);
const TOK_OP = new Set(["PLUS","MINUS","MUL","DIV","POW","LE","GT","LEQ","GEQ","EQ","NE",
  "AND","OR","NOT","ASSIGN","AMP"]);
function tokClass(t) {
  if (t === "NUM" || t === "FLOATNUM") return "n";
  if (t === "STRING") return "s";
  if (t === "ID") return "i";
  if (TOK_KW.has(t)) return "k";
  if (TOK_OP.has(t)) return "o";
  return "p";
}
function renderTokens(tokens) {
  const el = $("tokens");
  el.innerHTML = "";
  (tokens || []).forEach((tk) => {
    if (tk.type === "END") return;
    const c = document.createElement("span");
    c.className = "chip " + tokClass(tk.type);
    c.innerHTML = `<span class="ty">${tk.type}</span><span class="lx">${escapeHtml(tk.text)}</span>`;
    el.appendChild(c);
  });
  $("tokCount").textContent = (tokens || []).filter((t) => t.type !== "END").length || "";
}

function escapeHtml(s) {
  return (s || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}
function countNodes(n) { return n ? 1 + (n.children || []).reduce((a, c) => a + countNodes(c), 0) : 0; }

// ── Resultado ──
function renderResult(d) {
  const status = $("resultStatus"), out = $("output");
  if (d.error) {
    status.innerHTML = `<span class="pill err">error</span><span style="color:var(--text-muted)">Fallo en fase: ${d.errorStage || "—"}</span>`;
    out.className = "result-out";
    out.innerHTML = `<span style="color:var(--danger)">${escapeHtml(d.error)}</span>`;
    return;
  }
  const ok = d.exitCode === 0;
  status.innerHTML =
    `<span class="pill ${ok ? "ok" : "warn"}">${ok ? "ejecución correcta" : "código " + d.exitCode}</span>` +
    `<span style="color:var(--text-muted)">binario nativo x86-64</span>`;
  out.className = "result-out" + (d.output ? "" : " empty");
  out.textContent = d.output && d.output.length ? d.output : "(sin salida)";
}

// ── Compilar ──
async function run() {
  const btn = $("btnRun");
  btn.disabled = true;
  btn.innerHTML = `<span class="spinner"></span> Compilando…`;
  try {
    const r = await fetch("/api/compile", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ code: code.value }),
    });
    const d = await r.json();
    renderPipeline(d.stages || {});
    renderTokens(d.tokens);
    lastAsm = d.asm || "";
    $("asm").innerHTML = d.asm ? window.highlightAsm(d.asm) : "<span style='color:var(--text-faint)'>—</span>";
    if (d.ast) { window.AST.render($("astSvg"), d.ast); $("astCount").textContent = countNodes(d.ast); }
    else { $("astSvg").innerHTML = ""; $("astCount").textContent = ""; }
    renderResult(d);
    // ir a la pestaña de resultado (o a la del error)
    const target = d.error ? (["lexico"].includes(d.errorStage) ? "tokens" : d.errorStage === "generacion" ? "asm" : "result") : "result";
    document.querySelector(`.tab[data-tab="${target}"]`)?.click();
  } catch (e) {
    renderResult({ error: "No se pudo contactar el servidor: " + e });
  } finally {
    btn.disabled = false;
    btn.innerHTML = "▶ Compilar y ejecutar";
  }
}
$("btnRun").onclick = run;

// ── Ejemplos ──
function buildExamples() {
  const grid = $("exampleGrid");
  grid.innerHTML = "";
  (window.EXAMPLES || []).forEach((ex) => {
    const card = document.createElement("button");
    card.className = "ex-card";
    card.innerHTML = `<h4>${ex.title}</h4><p>${ex.desc}</p>` +
      `<div class="ex-tags">${ex.tags.map((t) => `<span class="ex-tag">${t}</span>`).join("")}</div>`;
    card.onclick = () => {
      code.value = ex.code;
      refreshEditor();
      $("overlay").classList.remove("show");
      run();
    };
    grid.appendChild(card);
  });
}
$("btnExamples").onclick = () => $("overlay").classList.add("show");
$("btnCloseEx").onclick = () => $("overlay").classList.remove("show");
$("overlay").onclick = (e) => { if (e.target === $("overlay")) $("overlay").classList.remove("show"); };

// ── Tema ──
$("btnTheme").onclick = () => {
  const html = document.documentElement;
  html.dataset.theme = html.dataset.theme === "dark" ? "light" : "dark";
  if (lastAsm) window.AST.fit();
};

// ── Descargar .s ──
$("btnDownload").onclick = () => {
  if (!lastAsm) return;
  const blob = new Blob([lastAsm], { type: "text/plain" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = "programa.s";
  a.click();
  URL.revokeObjectURL(a.href);
};

// ── Controles del AST ──
$("astZoomIn").onclick = () => window.AST.zoom(1.2);
$("astZoomOut").onclick = () => window.AST.zoom(0.8);
$("astFit").onclick = () => window.AST.fit();

// ── Inicio ──
code.value = (window.EXAMPLES && window.EXAMPLES[1] ? window.EXAMPLES[1].code : "fun int main()\n    print(42);\n    return(0)\nendfun");
buildExamples();
refreshEditor();
window.addEventListener("load", run);
