"""SanghaviMart LOCAL DEMO server — same /api/v1 contract as the C++ backend,
same Postgres, same Argon2id hashing, same HS256 JWTs. The frontend cannot
tell the difference. (Deployable backend remains the Drogon C++ binary;
this file is the localhost try-it-now path: run-local.bat)."""
import base64, hashlib, hmac, json, os, re, threading, time, urllib.parse, webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import psycopg
from argon2 import PasswordHasher
from argon2.exceptions import VerifyMismatchError

PORT = int(os.environ.get("PORT", "8080"))
DSN = os.environ.get("DATABASE_URL") or os.environ.get("POSTGRES_URL") or \
    "host=127.0.0.1 port=5434 dbname=sanghavimart user=sanghavi password=sanghavi_pw"
JWT_SECRET = os.environ.get("JWT_SECRET") or "dev-only-16plus-chars-change-me"
PUBLIC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "public")
ph = PasswordHasher()  # Argon2id, like crypto_pwhash in the C++ backend

VALID = {("PENDING", "CONFIRMED"), ("PENDING", "CANCELLED"), ("CONFIRMED", "SHIPPED"),
         ("CONFIRMED", "CANCELLED"), ("SHIPPED", "DELIVERED")}

# ---------- helpers ----------
def b64e(b: bytes) -> str:
    return base64.urlsafe_b64encode(b).rstrip(b"=").decode()

def issue(uid, role):
    h = b64e(b'{"alg":"HS256","typ":"JWT"}')
    p = b64e(json.dumps({"sub": uid, "role": role, "exp": int(time.time()) + 43200}).encode())
    s = b64e(hmac.new(JWT_SECRET.encode(), f"{h}.{p}".encode(), hashlib.sha256).digest())
    return f"{h}.{p}.{s}"

def db():
    c = psycopg.connect(DSN)
    c.autocommit = True
    return c

def ujson(row):
    return {"id": row[0], "name": row[1], "email": row[2], "role": row[3]}

def pjson(r):
    return {"id": r[0], "seller_id": r[1], "name": r[2], "description": r[3],
            "price_cents": r[4], "stock_qty": r[5], "category": r[6], "image_url": r[7]}

FAQ = [("return", "SanghaviMart returns: unused items can be returned within 7 days."),
       ("ship", "Orders flow Pending -> Confirmed -> Shipped -> Delivered (3-5 days)."),
       ("track", "Track your order from the Orders page; each order shows live status."),
       ("sell", "Register as SELLER, then manage listings from the Seller Dashboard."),
       ("pay", "Checkout uses a mock payment step — no real gateway."),
       ("refund", "Cancelled orders are refunded instantly to the mock-payment method."),
       ("coupon", "No coupons right now — the catalogue price is the price."),
       ("discount", "No coupons right now — the catalogue price is the price."),
       ("support", "Reach support at support@sanghavimart.local."),
       ("contact", "Reach support at support@sanghavimart.local."),
       ("hello", "Hello! Ask me about products, orders, shipping, returns or selling."),
       ("hi", "Hello! Ask me about products, orders, shipping, returns or selling.")]

def mock_reply(msg):
    m = msg.lower()
    for k, v in FAQ:
        if k in m:
            return v
    return "I can help with products, orders, shipping, returns and selling on SanghaviMart."

class H(BaseHTTPRequestHandler):
    server_version = "SanghaviMartDemo/1.0"

    def log_message(self, *a):
        pass

    def body(self):
        try:
            return json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0)) or 0) or b"{}")
        except Exception:
            return {}

    def send(self, code, data=None, err=None):
        payload = {"success": err is None, "data": data,
                   "error": None if err is None else {"code": err[0], "message": err[1]}}
        raw = json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def ident(self):
        """JWT-first identity (mirrors ServiceLocator::Identity)."""
        h = self.headers.get("Authorization", "")
        if h.startswith("Bearer "):
            try:
                p1, p2 = h[7:].split(".")[0], h[7:].split(".")[1]
                sig = h[7:].split(".")[2]
                want = hmac.new(JWT_SECRET.encode(), f"{p1}.{p2}".encode(), hashlib.sha256).digest()
                got = base64.urlsafe_b64decode(sig + "=" * (-len(sig) % 4))
                assert hmac.compare_digest(want, got)
                cl = json.loads(base64.urlsafe_b64decode(p2 + "=" * (-len(p2) % 4)))
                assert cl["exp"] >= time.time()
                return cl["sub"], cl["role"]
            except Exception:
                pass
        return 0, ""

    # ----- routing -----
    def do_GET(self):
        self.route("GET")

    def do_POST(self):
        self.route("POST")

    def do_PUT(self):
        self.route("PUT")

    def do_DELETE(self):
        self.route("DELETE")

    def route(self, m):
        pu = urllib.parse.urlparse(self.path)
        path, q = pu.path, urllib.parse.parse_qs(pu.query)
        c = db()
        try:
            self.dispatch(c, m, path, q)
        except Exception as e:  # never leak internals
            self.send(500, err=("INTERNAL_ERROR", "An unexpected error occurred"))
        finally:
            c.close()

    def dispatch(self, c, m, path, q):
        uid, role = self.ident()
        need = lambda r=None: self._need(uid, role, r)

        if path == "/api/v1/health":
            c.execute("SELECT 1")
            return self.send(200, {"status": "UP", "db": "UP"})
        if path == "/api/v1/auth/register" and m == "POST":
            j = self.body()
            for f in ("name", "email", "password", "role"):
                if not str(j.get(f, "")).strip():
                    return self.send(400, err=("VALIDATION_ERROR", f"{f} must not be empty"))
            if "@" not in j["email"] or len(j["password"]) < 8:
                return self.send(400, err=("VALIDATION_ERROR", "invalid email or weak password"))
            if j["role"] not in ("BUYER", "SELLER"):
                return self.send(400, err=("VALIDATION_ERROR", "role must be BUYER or SELLER"))
            try:
                r = c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s) RETURNING id,name,email,role",
                              (j["name"].strip(), j["email"].strip(), ph.hash(j["password"]), j["role"])).fetchone()
            except psycopg.errors.UniqueViolation:
                return self.send(409, err=("CONFLICT", "email already registered"))
            d = ujson(r)
            d["token"] = issue(r[0], r[3])
            return self.send(201, d)
        if path == "/api/v1/auth/login" and m == "POST":
            j = self.body()
            r = c.execute("SELECT id,name,email,password_hash,role FROM users WHERE email=%s", (j.get("email", ""),)).fetchone()
            try:
                assert r is not None
                ph.verify(r[3], j.get("password", ""))
            except (AssertionError, VerifyMismatchError):
                return self.send(401, err=("UNAUTHORIZED", "invalid email or password"))
            d = {"id": r[0], "name": r[1], "email": r[2], "role": r[4], "token": issue(r[0], r[4])}
            return self.send(200, d)
        if path == "/api/v1/auth/logout" and m == "POST":
            return self.send(200, {"ok": True})
        if path == "/api/v1/auth/me" and m == "GET":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            r = c.execute("SELECT id,name,email,role FROM users WHERE id=%s", (uid,)).fetchone()
            if not r:
                return self.send(401, err=("UNAUTHORIZED", "session invalid"))
            return self.send(200, ujson(r))
        if path == "/api/v1/products" and m == "GET":
            kw, cat = q.get("q", [""])[0], q.get("category", [""])[0]
            try:
                lim = max(1, min(int(q.get("limit", ["50"])[0]), 100))
            except ValueError:
                lim = 50
            like = f"%{kw}%"
            if cat:
                rows = c.execute("SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url FROM products WHERE category=%s AND (%s=%s OR name LIKE %s OR description LIKE %s) ORDER BY id LIMIT %s",
                                 (cat, like, "%%", like, like, lim)).fetchall()
            else:
                rows = c.execute("SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url FROM products WHERE (%s=%s OR name LIKE %s OR description LIKE %s) ORDER BY id LIMIT %s",
                                 (like, "%%", like, like, lim)).fetchall()
            return self.send(200, [pjson(r) for r in rows])
        if path == "/api/v1/products" and m == "POST":
            if err := need("SELLER"):
                return err
            j = self.body()
            if not str(j.get("name", "")).strip() or int(j.get("price_cents", 0)) <= 0 or int(j.get("stock_qty", 0)) < 0:
                return self.send(400, err=("VALIDATION_ERROR", "bad name/price/stock"))
            r = c.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url) VALUES(%s,%s,%s,%s,%s,%s,%s) RETURNING id,seller_id,name,description,price_cents,stock_qty,category,image_url",
                          (uid, j["name"].strip(), j.get("description", ""), j["price_cents"], j["stock_qty"], j.get("category", "general"), j.get("image_url", ""))).fetchone()
            return self.send(201, pjson(r))
        mt = re.match(r"^/api/v1/products/(\d+)$", path)
        if mt:
            pid = int(mt.group(1))
            if m == "GET":
                r = c.execute("SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url FROM products WHERE id=%s", (pid,)).fetchone()
                return self.send(200, pjson(r)) if r else self.send(404, err=("NOT_FOUND", "product not found"))
            if m in ("PUT", "DELETE"):
                if err := need("SELLER"):
                    return err
                cur = c.execute("SELECT seller_id FROM products WHERE id=%s", (pid,)).fetchone()
                if not cur:
                    return self.send(404, err=("NOT_FOUND", "product not found"))
                if cur[0] != uid and role != "ADMIN":
                    return self.send(403, err=("FORBIDDEN", "not your listing"))
                if m == "DELETE":
                    c.execute("DELETE FROM products WHERE id=%s", (pid,))
                    return self.send(200, {"ok": True})
                j = self.body()
                c.execute("UPDATE products SET name=%s,description=%s,price_cents=%s,stock_qty=%s,category=%s,image_url=%s WHERE id=%s",
                          (j["name"], j.get("description", ""), j["price_cents"], j["stock_qty"], j.get("category", "general"), j.get("image_url", ""), pid))
                r = c.execute("SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url FROM products WHERE id=%s", (pid,)).fetchone()
                return self.send(200, pjson(r))
        if path == "/api/v1/seller/products" and m == "GET":
            if err := need("SELLER"):
                return err
            rows = c.execute("SELECT id,seller_id,name,description,price_cents,stock_qty,category,image_url FROM products WHERE seller_id=%s ORDER BY id", (uid,)).fetchall()
            return self.send(200, [pjson(r) for r in rows])
        if path == "/api/v1/seller/dashboard" and m == "GET":
            if err := need("SELLER"):
                return err
            n = c.execute("SELECT COUNT(*) FROM order_items i JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s", (uid,)).fetchone()[0]
            rev = c.execute("SELECT COALESCE(SUM(i.quantity*i.unit_price_cents),0) FROM order_items i JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s", (uid,)).fetchone()[0]
            return self.send(200, {"order_count": n, "revenue_cents": int(rev)})
        if path == "/api/v1/seller/orders" and m == "GET":
            if err := need("SELLER"):
                return err
            rows = c.execute("SELECT DISTINCT o.id,o.status,o.total_amount_cents,o.created_at FROM orders o JOIN order_items i ON i.order_id=o.id JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s ORDER BY o.id DESC", (uid,)).fetchall()
            return self.send(200, [{"id": r[0], "status": r[1], "total_amount_cents": r[2], "created_at": str(r[3])} for r in rows])
        if path == "/api/v1/cart" and m == "GET":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            rows = c.execute("SELECT c.product_id,c.quantity,p.name,p.price_cents FROM cart_items c JOIN products p ON p.id=c.product_id WHERE c.user_id=%s ORDER BY c.id", (uid,)).fetchall()
            items = [{"product_id": r[0], "quantity": r[1], "name": r[2], "unit_price_cents": r[3], "line_total_cents": r[1] * r[3]} for r in rows]
            return self.send(200, {"items": items, "total_cents": sum(i["line_total_cents"] for i in items)})
        if path == "/api/v1/cart" and m == "POST":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            j = self.body()
            p = c.execute("SELECT stock_qty FROM products WHERE id=%s", (j.get("product_id"),)).fetchone()
            if not p:
                return self.send(404, err=("NOT_FOUND", "product not found"))
            if int(j.get("quantity", 0)) <= 0 or p[0] < int(j["quantity"]):
                return self.send(409, err=("CONFLICT", "bad quantity or insufficient stock"))
            c.execute("INSERT INTO cart_items(user_id,product_id,quantity) VALUES(%s,%s,%s) ON CONFLICT(user_id,product_id) DO UPDATE SET quantity=%s",
                      (uid, j["product_id"], j["quantity"], j["quantity"]))
            return self.send(201, {"ok": True})
        mt = re.match(r"^/api/v1/cart/(\d+)$", path)
        if mt:
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            pid = int(mt.group(1))
            if m == "DELETE":
                c.execute("DELETE FROM cart_items WHERE user_id=%s AND product_id=%s", (uid, pid))
                return self.send(200, {"ok": True})
            if m == "PUT":
                qty = int(self.body().get("quantity", 0))
                if qty <= 0:
                    c.execute("DELETE FROM cart_items WHERE user_id=%s AND product_id=%s", (uid, pid))
                else:
                    c.execute("UPDATE cart_items SET quantity=%s WHERE user_id=%s AND product_id=%s", (qty, uid, pid))
                return self.send(200, {"ok": True})
        if path == "/api/v1/orders/checkout" and m == "POST":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            items = c.execute("SELECT c.product_id,c.quantity,p.price_cents,p.stock_qty FROM cart_items c JOIN products p ON p.id=c.product_id WHERE c.user_id=%s", (uid,)).fetchall()
            if not items:
                return self.send(409, err=("CONFLICT", "cart is empty"))
            total = sum(qt * pr for _, qt, pr, _ in items)
            with c.transaction():
                oid = c.execute("INSERT INTO orders(buyer_id,status,total_amount_cents) VALUES(%s,'PENDING',%s) RETURNING id", (uid, total)).fetchone()[0]
                for pid_, qt, pr, st in items:
                    if st < qt:
                        raise RuntimeError("stock")
                    c.execute("INSERT INTO order_items(order_id,product_id,quantity,unit_price_cents) VALUES(%s,%s,%s,%s)", (oid, pid_, qt, pr))
                    c.execute("UPDATE products SET stock_qty=stock_qty-%s WHERE id=%s", (qt, pid_))
                c.execute("DELETE FROM cart_items WHERE user_id=%s", (uid,))
            return self.send(201, {"id": oid, "status": "PENDING", "total_amount_cents": total})
        if path == "/api/v1/orders" and m == "GET":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            rows = c.execute("SELECT id,status,total_amount_cents,created_at FROM orders WHERE buyer_id=%s ORDER BY id DESC", (uid,)).fetchall()
            return self.send(200, [{"id": r[0], "status": r[1], "total_amount_cents": r[2], "created_at": str(r[3])} for r in rows])
        mt = re.match(r"^/api/v1/orders/(\d+)/status$", path)
        if mt and m == "PUT":
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            to = self.body().get("status", "")
            o = c.execute("SELECT buyer_id,status FROM orders WHERE id=%s", (int(mt.group(1)),)).fetchone()
            if not o:
                return self.send(404, err=("NOT_FOUND", "order not found"))
            if role != "ADMIN" and o[0] != uid:
                mine = c.execute("SELECT COUNT(*) FROM order_items i JOIN products p ON p.id=i.product_id WHERE i.order_id=%s AND p.seller_id=%s", (int(mt.group(1)), uid)).fetchone()[0]
                if not mine:
                    return self.send(403, err=("FORBIDDEN", "no access to order"))
            if (o[1], to) not in VALID:
                return self.send(409, err=("CONFLICT", f"illegal transition {o[1]} -> {to}"))
            c.execute("UPDATE orders SET status=%s WHERE id=%s", (to, int(mt.group(1))))
            return self.send(200, {"ok": True})
        mt = re.match(r"^/api/v1/products/(\d+)/reviews$", path)
        if mt:
            pid = int(mt.group(1))
            if m == "GET":
                rows = c.execute("SELECT id,rating,comment,created_at FROM reviews WHERE product_id=%s ORDER BY id DESC", (pid,)).fetchall()
                return self.send(200, [{"id": r[0], "rating": r[1], "comment": r[2], "created_at": str(r[3])} for r in rows])
            if not uid:
                return self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            j = self.body()
            if int(j.get("rating", 0)) not in (1, 2, 3, 4, 5) or not str(j.get("comment", "")).strip():
                return self.send(400, err=("VALIDATION_ERROR", "rating 1..5 + comment required"))
            gate = c.execute("SELECT COUNT(*) FROM orders o JOIN order_items i ON i.order_id=o.id WHERE o.buyer_id=%s AND i.product_id=%s AND o.status!='CANCELLED'", (uid, pid)).fetchone()[0]
            if not gate:
                return self.send(403, err=("FORBIDDEN", "reviews allowed only on ordered products"))
            rid = c.execute("INSERT INTO reviews(product_id,user_id,rating,comment) VALUES(%s,%s,%s,%s) RETURNING id", (pid, uid, j["rating"], j["comment"])).fetchone()[0]
            return self.send(201, {"id": rid})
        if path == "/api/v1/admin/users" and m == "GET":
            if err := need("ADMIN"):
                return err
            rows = c.execute("SELECT id,name,email,role FROM users ORDER BY id").fetchall()
            return self.send(200, [ujson(r) for r in rows])
        if path == "/api/v1/admin/orders" and m == "GET":
            if err := need("ADMIN"):
                return err
            rows = c.execute("SELECT id,buyer_id,status,total_amount_cents FROM orders ORDER BY id DESC").fetchall()
            return self.send(200, [{"id": r[0], "buyer_id": r[1], "status": r[2], "total_amount_cents": r[3]} for r in rows])
        mt = re.match(r"^/api/v1/admin/products/(\d+)$", path)
        if mt and m == "DELETE":
            if err := need("ADMIN"):
                return err
            c.execute("DELETE FROM products WHERE id=%s", (int(mt.group(1)),))
            return self.send(200, {"ok": True})
        if path == "/api/v1/chat" and m == "POST":
            msg = str(self.body().get("message", ""))
            if not msg or len(msg) > 500:
                return self.send(400, err=("VALIDATION_ERROR", "message must be 1..500 chars"))
            key = f"u{uid}" if uid else "anon"
            now = int(time.time())
            c.execute("DELETE FROM chat_hits WHERE session_key=%s AND hit_at_sec < %s", (key, now - 60))
            if c.execute("SELECT COUNT(*) FROM chat_hits WHERE session_key=%s", (key,)).fetchone()[0] >= 10:
                return self.send(409, err=("CONFLICT", "rate limit: 10 messages/minute"))
            c.execute("INSERT INTO chat_hits(session_key,hit_at_sec) VALUES(%s,%s)", (key, now))
            nkey = msg.lower()
            r = c.execute("SELECT reply FROM chat_cache WHERE session_key=%s AND question=%s", (key, nkey)).fetchone()
            if r:
                return self.send(200, {"reply": r[0]})
            reply = mock_reply(msg)
            c.execute("INSERT INTO chat_cache(session_key,question,reply) VALUES(%s,%s,%s) ON CONFLICT(session_key,question) DO NOTHING", (key, nkey, reply))
            return self.send(200, {"reply": reply})
        # static frontend
        if m == "GET" and not path.startswith("/api/"):
            f = os.path.join(PUBLIC, "index.html" if path == "/" else path.lstrip("/").replace("..", ""))
            if os.path.isfile(f):
                ct = {"html": "text/html", "js": "text/javascript", "css": "text/css"}.get(f.rsplit(".", 1)[-1], "text/plain")
                raw = open(f, "rb").read()
                self.send_response(200)
                self.send_header("Content-Type", ct)
                self.send_header("Content-Length", str(len(raw)))
                self.end_headers()
                self.wfile.write(raw)
                return
        return self.send(404, err=("NOT_FOUND", "no such endpoint"))

    def _need(self, uid, role, want):
        if not uid:
            self.send(401, err=("UNAUTHORIZED", "Authentication required"))
            return True
        if want and role != want and role != "ADMIN":
            self.send(403, err=("FORBIDDEN", f"requires role {want}"))
            return True
        return False

def ensure_admin():
    c = db()
    r = c.execute("SELECT password_hash FROM users WHERE email='admin@sanghavimart.local'").fetchone()
    if r is None:
        c.execute("INSERT INTO users(name,email,password_hash,role) VALUES('SanghaviMart Admin','admin@sanghavimart.local',%s,'ADMIN')", (ph.hash("Admin123!"),))
        print("demo admin created: admin@sanghavimart.local / Admin123!")
    elif "REPLACE_ME" in r[0]:
        c.execute("UPDATE users SET password_hash=%s WHERE email='admin@sanghavimart.local'", (ph.hash("Admin123!"),))
        print("demo admin password set: admin@sanghavimart.local / Admin123!")
    s = c.execute("SELECT password_hash FROM users WHERE email='seller@sanghavimart.local'").fetchone()
    if s is None:
        c.execute("INSERT INTO users(name,email,password_hash,role) VALUES('Demo Seller','seller@sanghavimart.local',%s,'SELLER')", (ph.hash("Seller123!"),))
        print("demo seller created: seller@sanghavimart.local / Seller123!")
    elif "REPLACE_ME" in s[0]:
        c.execute("UPDATE users SET password_hash=%s WHERE email='seller@sanghavimart.local'", (ph.hash("Seller123!"),))
        print("demo seller password set: seller@sanghavimart.local / Seller123!")
    c.close()

if __name__ == "__main__":
    ensure_admin()
    srv = ThreadingHTTPServer(("0.0.0.0", PORT), H)
    print(f"SanghaviMart demo live at http://localhost:{PORT}  (same API + same Postgres as the C++ backend)")
    threading.Timer(1.0, lambda: webbrowser.open(f"http://localhost:{PORT}")).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")
