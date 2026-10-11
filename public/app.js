// SanghaviMart storefront (vanilla JS + fetch AJAX). All API calls under /api/v1.
// User output is rendered via textContent (escaped) � never innerHTML with user data.
const $ = (s) => document.querySelector(s);
const app = $("#app");
let me = null;
const TOKEN_KEY = "sm_token"; // stateless JWT: survives Vercel scale-to-zero (no server session needed)
function authHeaders() {
  const t = localStorage.getItem(TOKEN_KEY);
  return t ? { "Authorization": "Bearer " + t } : {};
}
async function api(path, opts = {}) {
  const headers = { "Content-Type": "application/json", ...authHeaders(), ...((opts && opts.headers) || {}) };
  const r = await fetch(path, { ...opts, headers });
  const j = await r.json();
  if (!j.success) {
    if (r.status === 401) localStorage.removeItem(TOKEN_KEY); // stale/expired token: drop it
    throw new Error((j.error && j.error.message) || ("HTTP " + r.status));
  }
  return j.data;
}
function esc(s) { return String(s == null ? "" : s); }
function el(tag, text) { const e = document.createElement(tag); e.textContent = text; return e; }
async function refreshMe() { try { me = await api("/api/v1/auth/me"); } catch { me = null; } }
function money(c) { return "Rs." + (c / 100).toFixed(2); }
const VIEWS = {
  async home() {
    const CATS = ["", "electronics", "fashion", "footwear", "home", "kitchen", "grocery", "beauty", "sports", "books", "toys", "appliances"];
    app.innerHTML = "";
    const bar = document.createElement("div");
    const q = document.createElement("input"); q.placeholder = "Search products..."; q.style.minWidth = "180px";
    const cat = document.createElement("select");
    for (const c of CATS) { const o = document.createElement("option"); o.value = c; o.textContent = c === "" ? "All categories" : c; cat.append(o); }
    const go = document.createElement("button"); go.textContent = "Search"; go.style.marginLeft = "6px";
    bar.append(q, cat, go); app.append(bar);
    const g = document.createElement("div"); g.className = "grid"; app.append(g);
    async function load() {
      const items = await api("/api/v1/products?limit=100&q=" + encodeURIComponent(q.value) + "&category=" + encodeURIComponent(cat.value));
      g.innerHTML = "";
      const n = document.createElement("p"); n.textContent = items.length + " products"; n.style.color = "var(--ink-soft)"; g.append(n);
      for (const p of items) {
        const c = document.createElement("div"); c.className = "card";
        if (p.image_url) { const im = document.createElement("img"); im.src = p.image_url; im.alt = p.name; im.loading = "lazy"; c.append(im); }
        c.append(el("h3", p.name), el("div", p.description), el("div", money(p.price_cents) + " | stock " + p.stock_qty));
        const b = document.createElement("button"); b.textContent = "Add to cart";
        b.onclick = async () => { try { await api("/api/v1/cart", { method: "POST", body: JSON.stringify({ product_id: p.id, quantity: 1 }) }); alert("added"); } catch (e) { alert(e.message); } };
        const r = document.createElement("button"); r.textContent = "Reviews"; r.style.marginLeft = "6px";
        r.onclick = () => VIEWS.reviews(p.id, p.name);
        c.append(b, r); g.append(c);
      }
    }
    go.onclick = load; q.onkeydown = (e) => { if (e.key === "Enter") load(); };
    await load();
  },
  async reviews(pid, name) {
    const rs = await api("/api/v1/products/" + pid + "/reviews");
    app.innerHTML = ""; app.append(el("h2", "Reviews � " + name));
    for (const r of rs) app.append(el("div", "*".repeat(r.rating) + " � " + r.comment));
    const cm = document.createElement("input"); cm.placeholder = "Write a review (must have ordered)";
    const go = document.createElement("button"); go.textContent = "Submit 5-star";
    go.onclick = async () => { try { await api("/api/v1/products/" + pid + "/reviews", { method: "POST", body: JSON.stringify({ rating: 5, comment: cm.value }) }); VIEWS.reviews(pid, name); } catch (e) { alert(e.message); } };
    app.append(cm, go);
  },
  async cart() {
    const c = await api("/api/v1/cart");
    app.innerHTML = ""; app.append(el("h2", "Cart � total " + money(c.total_cents)));
    const t = document.createElement("table");
    for (const i of c.items) {
      const tr = document.createElement("tr");
      tr.append(el("td", i.name), el("td", "x" + i.quantity), el("td", money(i.line_total_cents)));
      const rm = document.createElement("button"); rm.textContent = "Remove";
      rm.onclick = async () => { await api("/api/v1/cart/" + i.product_id, { method: "DELETE" }); VIEWS.cart(); };
      const td = document.createElement("td"); td.append(rm); tr.append(td); t.append(tr);
    }
    const co = document.createElement("button"); co.textContent = "Checkout (mock payment)";
    co.onclick = async () => { try { const o = await api("/api/v1/orders/checkout", { method: "POST", body: "{}" }); alert("order #" + o.id + " placed"); } catch (e) { alert(e.message); } };
    app.append(t, co);
  },
  async orders() {
    const os = await api("/api/v1/orders");
    app.innerHTML = ""; app.append(el("h2", "My orders"));
    for (const o of os) app.append(el("div", "#" + o.id + " " + o.status + " " + money(o.total_amount_cents)));
  },
  async seller() {
    if (!me || me.role !== "SELLER") { app.innerHTML = ""; app.append(el("p", "Login as SELLER to use the dashboard.")); return; }
    const [items, dash, incoming] = await Promise.all([
      api("/api/v1/seller/products"), api("/api/v1/seller/dashboard"), api("/api/v1/seller/orders")]);
    app.innerHTML = ""; app.append(el("h2", "Seller dashboard: " + dash.order_count + " sales, " + money(dash.revenue_cents)));
    const n = document.createElement("input"); n.placeholder = "name";
    const pr = document.createElement("input"); pr.placeholder = "price_cents"; pr.type = "number";
    const st = document.createElement("input"); st.placeholder = "stock"; st.type = "number";
    const add = document.createElement("button"); add.textContent = "Create listing";
    add.onclick = async () => { await api("/api/v1/products", { method: "POST", body: JSON.stringify({ name: n.value, description: "", price_cents: +pr.value, stock_qty: +st.value, category: "general", image_url: "" }) }); VIEWS.seller(); };
    app.append(n, pr, st, add);
    for (const p of items) {
      const d = document.createElement("div");
      d.append(el("span", "#" + p.id + " " + p.name + " " + money(p.price_cents) + " x" + p.stock_qty + " "));
      const del = document.createElement("button"); del.textContent = "Delete";
      del.onclick = async () => { await api("/api/v1/products/" + p.id, { method: "DELETE" }); VIEWS.seller(); };
      d.append(del); app.append(d);
    }
    app.append(el("h3", "Incoming orders"));
    for (const o of incoming) {
      const d = document.createElement("div"); d.append(el("span", "#" + o.id + " " + o.status + " "));
      for (const s of ["CONFIRMED", "SHIPPED", "DELIVERED"]) {
        const b = document.createElement("button"); b.textContent = s; b.style.marginLeft = "4px";
        b.onclick = async () => { try { await api("/api/v1/orders/" + o.id + "/status", { method: "PUT", body: JSON.stringify({ status: s }) }); VIEWS.seller(); } catch (e) { alert(e.message); } };
        d.append(b);
      }
      app.append(d);
    }
  },
  async admin() {
    try {
      const [us, os, ps] = await Promise.all([api("/api/v1/admin/users"), api("/api/v1/admin/orders"), api("/api/v1/products?limit=100")]);
      ps.sort((a, b) => b.id - a.id); // newest listings first: moderation starts at fresh abuse
      app.innerHTML = ""; app.append(el("h2", "Admin: " + us.length + " users, " + os.length + " orders"));
      app.append(el("h3", "Users"));
      for (const u of us) app.append(el("div", u.email + " (" + u.role + ")"));
      app.append(el("h3", "Orders"));
      for (const o of os) app.append(el("div", "#" + o.id + " " + o.status + " (" + o.buyer_id + ")"));
      app.append(el("h3", "Listings (" + ps.length + ") - Remove takes down any product"));
      for (const p of ps) {
        const d = document.createElement("div"); d.append(el("span", "#" + p.id + " " + p.name + " "));
        const del = document.createElement("button"); del.textContent = "Remove";
        del.onclick = async () => { try { await api("/api/v1/admin/products/" + p.id, { method: "DELETE" }); VIEWS.admin(); } catch (e) { alert(e.message); } };
        d.append(del); app.append(d);
      }
    } catch (e) { app.innerHTML = ""; app.append(el("p", "Admin only: " + e.message)); }
  },
  async auth() {
    app.innerHTML = "";
    app.append(el("h2", "Login or create your account"));
    app.append(el("p", "New here? Fill name + email + password, then choose Register as Buyer or Seller."));
    const e = document.createElement("input"); e.placeholder = "email";
    const p = document.createElement("input"); p.placeholder = "password"; p.type = "password";
    const n = document.createElement("input"); n.placeholder = "name (register)";
    const login = document.createElement("button"); login.textContent = "Login";
    login.onclick = async () => { try { const d = await api("/api/v1/auth/login", { method: "POST", body: JSON.stringify({ email: e.value, password: p.value }) }); if (d.token) localStorage.setItem(TOKEN_KEY, d.token); await boot(); } catch (er) { alert(er.message); } };
    const reg = document.createElement("button"); reg.textContent = "Register as Buyer"; reg.style.marginLeft = "6px";
    reg.onclick = async () => { try { const d = await api("/api/v1/auth/register", { method: "POST", body: JSON.stringify({ name: n.value, email: e.value, password: p.value, role: "BUYER" }) }); if (d.token) localStorage.setItem(TOKEN_KEY, d.token); await boot(); } catch (er) { alert(er.message); } };
    const regs = document.createElement("button"); regs.textContent = "Register as Seller"; regs.style.marginLeft = "6px";
    regs.onclick = async () => { try { const d = await api("/api/v1/auth/register", { method: "POST", body: JSON.stringify({ name: n.value, email: e.value, password: p.value, role: "SELLER" }) }); if (d.token) localStorage.setItem(TOKEN_KEY, d.token); await boot(); } catch (er) { alert(er.message); } };
    app.append(e, p, n, login, reg, regs);
  }
};
document.querySelectorAll("button[data-v]").forEach((b) => { b.onclick = () => VIEWS[b.dataset.v](); });
$("#logoutBtn").onclick = async () => {
  localStorage.removeItem(TOKEN_KEY);
  try { await api("/api/v1/auth/logout", { method: "POST", body: "{}" }); } catch { /* stateless: token drop is the real logout */ }
  await boot();
};
// Chat widget
$("#chatFab").onclick = () => $("#chatPanel").classList.toggle("open");
$("#chatForm").onsubmit = async (ev) => {
  ev.preventDefault();
  const inp = $("#chatIn"); const log = $("#chatLog");
  const t = inp.value.trim(); if (!t) return; inp.value = "";
  const m = document.createElement("div"); m.className = "msg me"; m.textContent = t; log.append(m);
  try { const d = await api("/api/v1/chat", { method: "POST", body: JSON.stringify({ message: t }) });
    const r = document.createElement("div"); r.className = "msg"; r.textContent = d.reply; log.append(r);
  } catch (e) { const r = document.createElement("div"); r.className = "msg"; r.textContent = "Sorry, chat is unavailable right now."; log.append(r); }
  log.scrollTop = log.scrollHeight;
};
async function boot() {
  await refreshMe();
  try { await VIEWS.home(); }
  catch {
    app.innerHTML = "";
    const w = document.createElement("h2"); w.textContent = "Server not reachable";
    const p = document.createElement("p");
    p.textContent = "Start it with run-local.bat (it picks a free port and opens the shop), then refresh this page.";
    app.append(w, p);
  }
}
boot();
