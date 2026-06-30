// Renderizador del AST como árbol SVG interactivo (sin dependencias).
(function () {
  const SVGNS = "http://www.w3.org/2000/svg";
  let state = { svg: null, g: null, k: 1, x: 0, y: 0, bounds: null };

  function categoryColor(label) {
    if (label.endsWith(":")) return "var(--node-field)";
    if (/^(Program|FunDec|VarDec|StructDec)/.test(label)) return "var(--node-decl)";
    if (/Stm$|^case |^default:/.test(label) || label.includes("Stm")) return "var(--node-stmt)";
    if (label.includes("Exp") || label.startsWith("new ")) return "var(--node-expr)";
    return "var(--node-leaf)";
  }

  function layout(root) {
    const charW = 6.6, padX = 16, levelH = 64;
    let maxW = 46, maxDepth = 0;
    (function measure(n) {
      n._w = Math.max(46, n.label.length * charW + padX);
      maxW = Math.max(maxW, n._w);
      (n.children || []).forEach(measure);
    })(root);
    const xGap = maxW + 16;
    let leaf = 0;
    const nodes = [], edges = [];
    (function assign(n, depth) {
      n._y = depth * levelH + 22;
      maxDepth = Math.max(maxDepth, depth);
      const ch = n.children || [];
      if (!ch.length) { n._x = leaf * xGap; leaf++; }
      else {
        ch.forEach((c) => assign(c, depth + 1));
        n._x = (ch[0]._x + ch[ch.length - 1]._x) / 2;
      }
      nodes.push(n);
      ch.forEach((c) => edges.push([n, c]));
    })(root, 0);
    return { nodes, edges, w: Math.max(1, leaf) * xGap + 40, h: (maxDepth + 1) * levelH + 40 };
  }

  function el(tag, attrs) {
    const e = document.createElementNS(SVGNS, tag);
    for (const k in attrs) e.setAttribute(k, attrs[k]);
    return e;
  }

  function render(svg, data) {
    state.svg = svg;
    svg.innerHTML = "";
    if (!data) return;
    const { nodes, edges, w, h } = layout(data);
    state.bounds = { w, h };
    const g = el("g", {});
    state.g = g;
    svg.appendChild(g);

    // aristas
    for (const [p, c] of edges) {
      const x1 = p._x, y1 = p._y + 11, x2 = c._x, y2 = c._y - 11;
      const my = (y1 + y2) / 2;
      g.appendChild(el("path", {
        class: "ast-edge",
        d: `M ${x1} ${y1} C ${x1} ${my}, ${x2} ${my}, ${x2} ${y2}`,
      }));
    }
    // nodos
    for (const n of nodes) {
      const col = categoryColor(n.label);
      const wbox = n._w, hbox = 22;
      const grp = el("g", { class: "ast-node", transform: `translate(${n._x - wbox / 2}, ${n._y - hbox / 2})` });
      grp.appendChild(el("rect", {
        width: wbox, height: hbox, rx: 7,
        fill: "var(--surface-2)", stroke: col,
      }));
      const txt = el("text", { x: wbox / 2, y: hbox / 2 + 4, "text-anchor": "middle", fill: col });
      txt.textContent = n.label;
      grp.appendChild(txt);
      g.appendChild(grp);
    }
    fit();
    enablePan(svg);
  }

  function apply() {
    if (state.g) state.g.setAttribute("transform", `translate(${state.x},${state.y}) scale(${state.k})`);
  }

  function fit() {
    if (!state.svg || !state.bounds) return;
    const r = state.svg.getBoundingClientRect();
    const k = Math.min(r.width / state.bounds.w, r.height / state.bounds.h, 1.4) * 0.92 || 1;
    state.k = k;
    state.x = (r.width - state.bounds.w * k) / 2;
    state.y = 14;
    apply();
  }

  function zoom(f) {
    state.k = Math.max(0.2, Math.min(3, state.k * f));
    apply();
  }

  function enablePan(svg) {
    let drag = false, sx = 0, sy = 0, ox = 0, oy = 0;
    svg.onmousedown = (e) => { drag = true; sx = e.clientX; sy = e.clientY; ox = state.x; oy = state.y; };
    window.addEventListener("mousemove", (e) => {
      if (!drag) return;
      state.x = ox + (e.clientX - sx); state.y = oy + (e.clientY - sy); apply();
    });
    window.addEventListener("mouseup", () => { drag = false; });
    svg.onwheel = (e) => { e.preventDefault(); zoom(e.deltaY < 0 ? 1.1 : 0.9); };
  }

  window.AST = { render, zoom, fit };
})();
