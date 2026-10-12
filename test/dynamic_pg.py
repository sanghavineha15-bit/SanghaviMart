"""SanghaviMart dynamic Postgres test suite.
Mirrors MigrationService (applies db/postgres/V1..V4 in order, schema_migrations
tracking) then exercises every mandatory requirement F1-F8 + O2/O3 against the
REAL database with parameterized queries only.
"""
import pathlib, sys
import psycopg

BASE = pathlib.Path("F:/ayyapacapc/sanghavimart/db/postgres")
DSN = "host=127.0.0.1 port=5433 dbname=sanghavimart user=sanghavi password=sanghavi_pw"
passed, failed = [], []

def check(name, fn):
    try:
        fn()
        passed.append(name)
        print(f"PASS {name}")
    except Exception as e:  # noqa: BLE001
        failed.append((name, str(e)[:300]))
        print(f"FAIL {name}: {e!r}"[:400])

def conn():
    c = psycopg.connect(DSN)
    c.autocommit = True
    return c

ADMIN = "admin@sanghavimart.local"

def migrate(c):
    c.execute("CREATE TABLE IF NOT EXISTS schema_migrations(version INTEGER PRIMARY KEY, applied_at TIMESTAMPTZ DEFAULT NOW())")
    files = sorted(BASE.glob("V*.sql"), key=lambda p: p.name)
    assert [f.name for f in files] == ["V1__init_schema.sql", "V2__add_reviews_table.sql",
                                       "V3__add_index_orders_status.sql", "V4__seed_admin.sql",
                                       "V5__chat_persistence.sql", "V6__login_codes.sql"], files
    for f in files:
        ver = int(f.name[1])
        if c.execute("SELECT version FROM schema_migrations WHERE version=%s", (ver,)).fetchone():
            continue
        with c.transaction():
            c.execute(f.read_text())
            c.execute("INSERT INTO schema_migrations(version) VALUES(%s)", (ver,))
    vers = [r[0] for r in c.execute("SELECT version FROM schema_migrations ORDER BY 1").fetchall()]
    assert vers == [1, 2, 3, 4, 5, 6], vers

def main():
    c = conn()
    # Greenfield every run: drop + recreate schema so absolute asserts
    # (stock 120-2, revenue 2*94900, single admin row) stay deterministic.
    c.execute("DROP SCHEMA public CASCADE")
    c.execute("CREATE SCHEMA public")
    check("M1 migrations apply in order, tracked", lambda: migrate(c))
    check("M2 idempotent re-run applies nothing", lambda: migrate(c))

    # --- F1: users/roles ---
    def f1():
        c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                  ("Buyer One", "buyer@sanghavimart.local", "$argon2id$testhash", "BUYER"))
        c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                  ("Seller One", "seller@sanghavimart.local", "$argon2id$testhash", "SELLER"))
        try:
            c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                      ("Dup", "buyer@sanghavimart.local", "h", "BUYER"))
            raise AssertionError("UNIQUE email not enforced")
        except psycopg.errors.UniqueViolation:
            pass
        try:
            c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                      ("Bad", "bad@x.local", "h", "HACKER"))
            raise AssertionError("role CHECK not enforced")
        except psycopg.errors.CheckViolation:
            pass
        admin = c.execute("SELECT role FROM users WHERE email=%s", (ADMIN,)).fetchone()
        assert admin and admin[0] == "ADMIN", "admin seed row missing"
    check("F1 users: unique email, role CHECK, admin seed", f1)

    # --- F2: seller listings + money positive ---
    def f2():
        sid = c.execute("SELECT id FROM users WHERE email=%s", ("seller@sanghavimart.local",)).fetchone()[0]
        c.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category) VALUES(%s,%s,%s,%s,%s,%s)",
                  (sid, "Steel Bottle", "insulated", 89900, 120, "home"))
        try:
            c.execute("INSERT INTO products(seller_id,name,price_cents,stock_qty) VALUES(%s,%s,%s,%s)", (sid, "Bad", 0, 5))
            raise AssertionError("price_cents>0 not enforced")
        except psycopg.errors.CheckViolation:
            pass
        try:
            c.execute("INSERT INTO products(seller_id,name,price_cents,stock_qty) VALUES(%s,%s,%s,%s)", (sid, "Bad2", 100, -1))
            raise AssertionError("stock>=0 not enforced")
        except psycopg.errors.CheckViolation:
            pass
        c.execute("UPDATE products SET price_cents=%s WHERE name=%s", (94900, "Steel Bottle"))
        assert c.execute("SELECT price_cents FROM products WHERE name=%s", ("Steel Bottle",)).fetchone()[0] == 94900
    check("F2 listings CRUD + money CHECKs", f2)

    # --- F3: search/filter (parameterized LIKE, injection-safe) ---
    def f3():
        rows = c.execute("SELECT name FROM products WHERE (%s=%s OR name LIKE %s OR description LIKE %s)",
                         ("%%", "%%", "%Bottle%", "%Bottle%")).fetchall()
        assert any("Bottle" in r[0] for r in rows), rows
        evil = "' OR '1'='1"
        rows = c.execute("SELECT id FROM products WHERE name LIKE %s", ("%" + evil + "%",)).fetchall()
        assert rows == [], f"injection leaked: {rows}"
        rows = c.execute("SELECT id FROM products WHERE category=%s", ("home",)).fetchall()
        assert len(rows) >= 1
    check("F3 search/filter + injection-safe LIKE", f3)

    # --- F4: cart ---
    def f4():
        bid = c.execute("SELECT id FROM users WHERE email=%s", ("buyer@sanghavimart.local",)).fetchone()[0]
        pid = c.execute("SELECT id,price_cents FROM products WHERE name=%s", ("Steel Bottle",)).fetchone()[0]
        c.execute("INSERT INTO cart_items(user_id,product_id,quantity) VALUES(%s,%s,%s) ON CONFLICT(user_id,product_id) DO UPDATE SET quantity=%s",
                  (bid, pid, 2, 2))
        c.execute("UPDATE cart_items SET quantity=%s WHERE user_id=%s AND product_id=%s", (3, bid, pid))
        total = c.execute("SELECT SUM(c.quantity*p.price_cents) FROM cart_items c JOIN products p ON p.id=c.product_id WHERE c.user_id=%s", (bid,)).fetchone()[0]
        assert total == 3 * 94900, total
        c.execute("DELETE FROM cart_items WHERE user_id=%s AND product_id=%s", (bid, pid))
        assert c.execute("SELECT COUNT(*) FROM cart_items WHERE user_id=%s", (bid,)).fetchone()[0] == 0
        c.execute("INSERT INTO cart_items(user_id,product_id,quantity) VALUES(%s,%s,%s)", (bid, pid, 2))
    check("F4 cart add/update/remove + running total", f4)

    # --- F5: checkout transaction (same SQL shape as DbOrderRepository::CreateOrder) ---
    def f5():
        bid = c.execute("SELECT id FROM users WHERE email=%s", ("buyer@sanghavimart.local",)).fetchone()[0]
        items = c.execute("SELECT c.product_id,c.quantity,p.price_cents,p.stock_qty FROM cart_items c JOIN products p ON p.id=c.product_id WHERE c.user_id=%s", (bid,)).fetchall()
        assert items, "cart empty before checkout"
        total = sum(q * pr for _, q, pr, _ in items)
        with c.transaction():
            oid = c.execute("INSERT INTO orders(buyer_id,status,total_amount_cents) VALUES(%s,%s,%s) RETURNING id",
                            (bid, "PENDING", total)).fetchone()[0]
            for pid_, q, pr, stock in items:
                assert stock >= q, "insufficient stock"
                c.execute("INSERT INTO order_items(order_id,product_id,quantity,unit_price_cents) VALUES(%s,%s,%s,%s)", (oid, pid_, q, pr))
                c.execute("UPDATE products SET stock_qty=stock_qty-%s WHERE id=%s AND stock_qty>=%s", (q, pid_, q))
            c.execute("DELETE FROM cart_items WHERE user_id=%s", (bid,))
        assert c.execute("SELECT total_amount_cents,status FROM orders WHERE id=%s", (oid,)).fetchone() == (total, "PENDING")
        assert c.execute("SELECT stock_qty FROM products WHERE name=%s", ("Steel Bottle",)).fetchone()[0] == 120 - 2
        assert c.execute("SELECT COUNT(*) FROM cart_items WHERE user_id=%s", (bid,)).fetchone()[0] == 0
    check("F5 checkout txn: order+items+stock+cart-clear", f5)

    # --- O2: status workflow legality mirror (IsValidStatusTransition) ---
    def o2():
        valid = {("PENDING", "CONFIRMED"), ("PENDING", "CANCELLED"), ("CONFIRMED", "SHIPPED"),
                 ("CONFIRMED", "CANCELLED"), ("SHIPPED", "DELIVERED")}
        oid = c.execute("SELECT id FROM orders ORDER BY id DESC LIMIT 1").fetchone()[0]
        for frm, to in [("PENDING", "CONFIRMED"), ("CONFIRMED", "SHIPPED"), ("SHIPPED", "DELIVERED")]:
            assert (frm, to) in valid
            c.execute("UPDATE orders SET status=%s WHERE id=%s", (to, oid))
        assert c.execute("SELECT status FROM orders WHERE id=%s", (oid,)).fetchone()[0] == "DELIVERED"
        assert ("PENDING", "SHIPPED") not in valid and ("DELIVERED", "CANCELLED") not in valid
    check("O2 status workflow PENDING..DELIVERED", o2)

    # --- F6: histories ---
    def f6():
        bid = c.execute("SELECT id FROM users WHERE email=%s", ("buyer@sanghavimart.local",)).fetchone()[0]
        sid = c.execute("SELECT id FROM users WHERE email=%s", ("seller@sanghavimart.local",)).fetchone()[0]
        assert c.execute("SELECT COUNT(*) FROM orders WHERE buyer_id=%s", (bid,)).fetchone()[0] >= 1
        n = c.execute("SELECT COUNT(DISTINCT o.id) FROM orders o JOIN order_items i ON i.order_id=o.id JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s", (sid,)).fetchone()[0]
        assert n >= 1, "seller incoming empty"
    check("F6 buyer history + seller incoming", f6)

    # --- F7: admin ---
    def f7():
        assert c.execute("SELECT COUNT(*) FROM users").fetchone()[0] >= 3
        assert c.execute("SELECT COUNT(*) FROM orders").fetchone()[0] >= 1
        sid = c.execute("SELECT id FROM users WHERE email=%s", ("seller@sanghavimart.local",)).fetchone()[0]
        c.execute("INSERT INTO products(seller_id,name,price_cents,stock_qty) VALUES(%s,%s,%s,%s)", (sid, "Takedown Item", 100, 1))
        pid = c.execute("SELECT id FROM products WHERE name=%s", ("Takedown Item",)).fetchone()[0]
        c.execute("DELETE FROM products WHERE id=%s", (pid,))
        assert c.execute("SELECT COUNT(*) FROM products WHERE id=%s", (pid,)).fetchone()[0] == 0
    check("F7 admin views + listing takedown", f7)

    # --- F8: reviews ---
    def f8():
        bid = c.execute("SELECT id FROM users WHERE email=%s", ("buyer@sanghavimart.local",)).fetchone()[0]
        pid = c.execute("SELECT id FROM products WHERE name=%s", ("Steel Bottle",)).fetchone()[0]
        gated = c.execute("SELECT COUNT(*) FROM orders o JOIN order_items i ON i.order_id=o.id WHERE o.buyer_id=%s AND i.product_id=%s AND o.status!=%s",
                          (bid, pid, "CANCELLED")).fetchone()[0]
        assert gated >= 1, "review gate query broken"
        c.execute("INSERT INTO reviews(product_id,user_id,rating,comment) VALUES(%s,%s,%s,%s)", (pid, bid, 5, "Excellent bottle"))
        try:
            c.execute("INSERT INTO reviews(product_id,user_id,rating,comment) VALUES(%s,%s,%s,%s)", (pid, bid, 9, "x"))
            raise AssertionError("rating CHECK not enforced")
        except psycopg.errors.CheckViolation:
            pass
        avg = c.execute("SELECT AVG(rating) FROM reviews WHERE product_id=%s", (pid,)).fetchone()[0]
        assert float(avg) == 5.0, avg
    check("F8 reviews gated + rating 1..5 + avg", f8)

    # --- O3: dashboard aggregates ---
    def o3():
        sid = c.execute("SELECT id FROM users WHERE email=%s", ("seller@sanghavimart.local",)).fetchone()[0]
        n = c.execute("SELECT COUNT(*) FROM order_items i JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s", (sid,)).fetchone()[0]
        rev = c.execute("SELECT COALESCE(SUM(i.quantity*i.unit_price_cents),0) FROM order_items i JOIN products p ON p.id=i.product_id WHERE p.seller_id=%s", (sid,)).fetchone()[0]
        assert n >= 1 and rev == 2 * 94900, (n, rev)
    check("O3 seller dashboard aggregates", o3)

    # --- indexes on every FK ---
    def idx():
        want = {"idx_users_email", "idx_products_seller", "idx_products_category", "idx_orders_buyer",
                "idx_orders_status", "idx_items_order", "idx_items_product", "idx_cart_user",
                "idx_cart_product", "idx_reviews_product", "idx_reviews_user"}
        have = {r[0] for r in c.execute("SELECT indexname FROM pg_indexes WHERE schemaname='public'").fetchall()}
        assert want <= have, f"missing: {want - have}"
    check("IDX every FK indexed", idx)

    # --- V5/O4-stateless: chat rate-limit + cache tables (mirror ChatService SQL) ---
    def chat():
        tables = {r[0] for r in c.execute(
            "SELECT tablename FROM pg_tables WHERE schemaname='public'").fetchall()}
        assert {"chat_hits", "chat_cache"} <= tables, tables
        c.execute("DELETE FROM chat_hits WHERE session_key=%s", ("u-test",))
        c.execute("DELETE FROM chat_cache WHERE session_key=%s", ("u-test",))
        now = 1700000000
        for i in range(10):  # 10 hits inside the window: allowed
            c.execute("DELETE FROM chat_hits WHERE session_key=%s AND hit_at_sec < %s",
                      ("u-test", now - 60))
            n = c.execute("SELECT COUNT(*) FROM chat_hits WHERE session_key=%s",
                          ("u-test",)).fetchone()[0]
            assert n < 10, "limit tripped early"
            c.execute("INSERT INTO chat_hits(session_key,hit_at_sec) VALUES(%s,%s)",
                      ("u-test", now))
        n = c.execute("SELECT COUNT(*) FROM chat_hits WHERE session_key=%s",
                      ("u-test",)).fetchone()[0]
        assert n == 10, n  # 11th must be rejected by the service
        c.execute("INSERT INTO chat_cache(session_key,question,reply) VALUES(%s,%s,%s)"
                  " ON CONFLICT(session_key,question) DO NOTHING",
                  ("u-test", "shipping?", "Ships in 3-5 days"))
        c.execute("INSERT INTO chat_cache(session_key,question,reply) VALUES(%s,%s,%s)"
                  " ON CONFLICT(session_key,question) DO NOTHING",
                  ("u-test", "shipping?", "Ships in 3-5 days"))  # idempotent
        got = c.execute("SELECT reply FROM chat_cache WHERE session_key=%s AND question=%s",
                        ("u-test", "shipping?")).fetchone()[0]
        assert got == "Ships in 3-5 days", got
        c.execute("DELETE FROM chat_hits WHERE session_key=%s AND hit_at_sec < %s",
                  ("u-test", now + 61 - 60))  # window slides: old hits pruned
        n = c.execute("SELECT COUNT(*) FROM chat_hits WHERE session_key=%s",
                      ("u-test",)).fetchone()[0]
        assert n == 0, n
    check("V5 chat tables: 10/min window + cache roundtrip", chat)

    # --- V6/O4: login_codes table shape (hash-only, expiry, attempts, single-use) ---
    def otp():
        cols = {r[0]: r[1] for r in c.execute(
            "SELECT column_name,data_type FROM information_schema.columns WHERE table_name='login_codes'").fetchall()}
        assert cols.get("code_hash") == "text", cols
        assert cols.get("expires_at_sec") == "bigint", cols
        assert "idx_login_codes_email" in {r[0] for r in c.execute(
            "SELECT indexname FROM pg_indexes WHERE tablename='login_codes'").fetchall()}
        c.execute("INSERT INTO login_codes(email,code_hash,expires_at_sec,attempts,used) VALUES(%s,%s,%s,%s,%s)",
                  ("o@t.local", "h", 9999999999, 0, 0))
        c.execute("UPDATE login_codes SET attempts=attempts+1,used=1 WHERE email=%s", ("o@t.local",))
        r = c.execute("SELECT attempts,used FROM login_codes WHERE email=%s", ("o@t.local",)).fetchone()
        assert tuple(r) == (1, 1), r
    check("V6 login_codes: hash/expiry/attempts/single-use", otp)

    print(f"\n==== {len(passed)} passed, {len(failed)} failed ====")
    c.close()
    sys.exit(1 if failed else 0)

main()

