"""Container boot (fast): prepare schema + demo users + catalog, then the
server starts. First boot seeds (~seconds); later boots are one COUNT query.
Must finish well inside Vercel's ~28s startup window."""
import json
import os
import pathlib
import sys

ROOT = pathlib.Path("/app") if pathlib.Path("/app/dblib.py").exists() else pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from dblib import DB, ensure_schema  # noqa: E402
from argon2 import PasswordHasher  # noqa: E402

PG_FILES = ["db/postgres/schema.sql", "db/postgres/V2__add_reviews_table.sql",
            "db/postgres/V3__add_index_orders_status.sql",
            "db/postgres/V5__chat_persistence.sql", "db/postgres/V6__login_codes.sql"]
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

db = DB()
if os.environ.get("DATABASE_URL") or os.environ.get("POSTGRES_URL"):
    try:
        db.execute("SELECT pg_advisory_lock(178231)")
    except Exception:
        pass  # SQLite has no advisory locks; single-process boot is safe there
    for f in PG_FILES:
        db.execute(pathlib.Path(ROOT, f).read_text(encoding="utf-8"))
    print("boot: postgres schema ensured", flush=True)
else:
    ensure_schema(db)
    print("boot: sqlite schema ensured", flush=True)

ph = PasswordHasher()
for name, email, pw, role in [("Demo Seller", "seller@sanghavimart.local", "Seller123!", "SELLER"),
                              ("SanghaviMart Admin", "admin@sanghavimart.local", "Admin123!", "ADMIN")]:
    r = db.execute("SELECT id FROM users WHERE email=%s", (email,)).fetchone()
    if r is None:
        db.execute("INSERT INTO users(name,email,password_hash,role) VALUES(%s,%s,%s,%s)",
                   (name, email, ph.hash(pw), role))
    else:
        db.execute("UPDATE users SET password_hash=%s WHERE email=%s", (ph.hash(pw), email))
sid = db.execute("SELECT id FROM users WHERE email='seller@sanghavimart.local'").fetchone()[0]

catalog = json.load(open(ROOT / "test" / "catalog_v2.json", encoding="utf-8"))
n = db.execute("SELECT COUNT(*) FROM products WHERE seller_id=%s", (sid,)).fetchone()[0]
if n < 300:
    db.execute("DELETE FROM products WHERE seller_id=%s", (sid,))
    for name, desc, price, stock, cat, img in SEEDS:
        db.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)"
                   " VALUES(%s,%s,%s,%s,%s,%s,%s)", (sid, name, desc, price, stock, cat, img))
    vals, params = [], []
    for p in catalog:
        vals.append("(%s,%s,%s,%s,%s,%s,%s)")
        params += [sid, p["name"], p["desc"], p["price"], p["stock"], p["category"], p["image"]]
    for i in range(0, len(vals), 100):  # chunked: safe under every param limit
        chunk_p = params[i * 7:(i + 100) * 7]
        db.execute("INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)"
                   " VALUES " + ",".join(vals[i:i + 100]), tuple(chunk_p))
    print(f"boot: seeded {len(catalog)} products", flush=True)
else:
    print(f"boot: catalog present ({n} rows), skip", flush=True)
db.close()
