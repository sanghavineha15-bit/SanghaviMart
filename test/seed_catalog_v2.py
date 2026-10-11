"""SanghaviMart catalog v2 seeder: 300+ products, every product its own unique,
human-verified photo. Idempotent (upsert on seller+name). Run:
  python test/seed_catalog_v2.py            # full reseed (replaces v1 catalog)
  python test/seed_catalog_v2.py --if-empty # no-op unless seller has <300 rows
Also self-heals demo logins (seller/admin with known passwords)."""
import json, os, sys
import psycopg
from argon2 import PasswordHasher

DSN = os.environ.get("DATABASE_URL") or os.environ.get("POSTGRES_URL") or \
    "host=127.0.0.1 port=5434 dbname=sanghavimart user=sanghavi password=sanghavi_pw"
HERE = os.path.dirname(os.path.abspath(__file__))
ph = PasswordHasher()

# (name, description, price_paise, stock, category, image) — photos verified live.
SEEDS = [
 ("Steel Water Bottle 1L", "Double-wall insulated bottle, 24h cold.", 89900, 120, "home",
  "https://thumb.wikimedia.org/wikipedia/commons/thumb/4/45/Metal_Water_Bottles.jpeg/960px-Metal_Water_Bottles.jpeg"),
 ("Cotton Kurta - Classic White", "Handloom cotton kurta, sizes S-XXL.", 129900, 60, "fashion",
  "https://thumb.wikimedia.org/wikipedia/commons/thumb/3/31/Kurta_traditional_front_sandalwood_buttons.jpg/960px-Kurta_traditional_front_sandalwood_buttons.jpg"),
 ("USB Mobile Charger", "Compact wall charger with fast-charge support.", 149900, 200, "electronics",
  "https://thumb.wikimedia.org/wikipedia/commons/thumb/c/c9/USB_power_adapter_for_Apple_iPod%2C_Model_A1205%2C_by_Foxlink_Technology_Ltd-1048.jpg/960px-USB_power_adapter_for_Apple_iPod%2C_Model_A1205%2C_by_Foxlink_Technology_Ltd-1048.jpg"),
 ("Filter Coffee Powder 500g", "80/20 coffee-chicory blend.", 34900, 300, "grocery",
  "https://thumb.wikimedia.org/wikipedia/commons/thumb/e/ee/Coffee_powdered.jpg/960px-Coffee_powdered.jpg"),
]
OLD_SEED_NAMES = {"Steel Water Bottle 1L", "Cotton Kurta - Indigo", "Cotton Kurta - Classic White",
                  "USB-C Fast Charger 65W", "USB Mobile Charger", "Filter Coffee Powder 500g"}

def ensure_user(c, name, email, password, role):
    r = c.execute("SELECT id,password_hash FROM users WHERE email=%s", (email,)).fetchone()
    if r is None:
        c.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                  (name, email, ph.hash(password), role))
        print(f"created {role.lower()} login: {email} / {password}")
    elif "REPLACE_ME" in r[1]:
        c.execute("UPDATE users SET password_hash=%s WHERE email=%s", (ph.hash(password), email))
        print(f"password set for demo login: {email} / {password}")

def main():
    c = psycopg.connect(DSN)
    c.autocommit = True
    ensure_user(c, "Demo Seller", "seller@sanghavimart.local", "Seller123!", "SELLER")
    ensure_user(c, "SanghaviMart Admin", "admin@sanghavimart.local", "Admin123!", "ADMIN")
    sid = c.execute("SELECT id FROM users WHERE email='seller@sanghavimart.local'").fetchone()[0]
    catalog = json.load(open(os.path.join(HERE, "catalog_v2.json"), encoding="utf-8"))
    if "--if-empty" in sys.argv:
        n = c.execute("SELECT COUNT(*) FROM products WHERE seller_id=%s", (sid,)).fetchone()[0]
        if n >= 300:
            print(f"catalog present ({n} rows), skipping")
            return
    keep = {p["name"] for p in catalog} | OLD_SEED_NAMES
    # v1 rows may own synthetic order history (tests/E2E): clear dependents first.
    # reviews/cart cascade automatically; order_items does not.
    c.execute("DELETE FROM order_items WHERE product_id IN (SELECT id FROM products WHERE seller_id=%s AND NOT (name = ANY(%s)))",
              (sid, list(keep)))
    c.execute("DELETE FROM orders WHERE id NOT IN (SELECT order_id FROM order_items)")
    c.execute("DELETE FROM products WHERE seller_id=%s AND NOT (name = ANY(%s))",
              (sid, list(keep)))
    for name, desc, price, stock, cat, img in SEEDS:
        old = ("Cotton Kurta - Indigo" if "Classic White" in name else
               "USB-C Fast Charger 65W" if name == "USB Mobile Charger" else name)
        r = c.execute("UPDATE products SET name=%s,description=%s,price_cents=%s,stock_qty=%s,category=%s,image_url=%s WHERE seller_id=%s AND (name=%s OR name=%s)",
                      (name, desc, price, stock, cat, img, sid, name, old))
        if not r.rowcount:
            c.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url) VALUES(%s,%s,%s,%s,%s,%s,%s)",
                      (sid, name, desc, price, stock, cat, img))
    ins = upd = 0
    for p in catalog:
        r = c.execute("UPDATE products SET description=%s,price_cents=%s,stock_qty=%s,category=%s,image_url=%s WHERE seller_id=%s AND name=%s",
                      (p["desc"], p["price"], p["stock"], p["category"], p["image"], sid, p["name"]))
        if r.rowcount:
            upd += 1
        else:
            c.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url) VALUES(%s,%s,%s,%s,%s,%s,%s)",
                      (sid, p["name"], p["desc"], p["price"], p["stock"], p["category"], p["image"]))
            ins += 1
    n, ncats, nimg = c.execute(
        "SELECT COUNT(*), COUNT(DISTINCT category), COUNT(DISTINCT image_url) FROM products WHERE seller_id=%s",
        (sid,)).fetchone()
    assert nimg == n, f"duplicate images! {nimg} unique of {n}"
    print(f"catalog v2: inserted={ins} updated={upd} | seller total={n} across {ncats} categories, {nimg} unique images")
    for cat, k in c.execute("SELECT category,COUNT(*) FROM products WHERE seller_id=%s GROUP BY 1 ORDER BY 1", (sid,)).fetchall():
        print(f"  {cat}: {k}")
    c.close()

main()
