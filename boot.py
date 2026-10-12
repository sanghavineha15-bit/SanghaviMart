"""Container boot: prepare the database, then exec the server.
- Postgres (DATABASE_URL set): apply db/postgres schema files (idempotent).
- SQLite (no DATABASE_URL): dblib.ensure_schema (ephemeral demo note in logs).
- Then seed demo users + catalog if empty (fast COUNT check first).
"""
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path("/app")
sys.path.insert(0, str(ROOT))
from dblib import DB, ensure_schema  # noqa: E402

PG_FILES = ["db/postgres/schema.sql", "db/postgres/V2__add_reviews_table.sql",
            "db/postgres/V3__add_index_orders_status.sql",
            "db/postgres/V5__chat_persistence.sql", "db/postgres/V6__login_codes.sql"]

db = DB()
if os.environ.get("DATABASE_URL") or os.environ.get("POSTGRES_URL"):
    for f in PG_FILES:
        db.execute(pathlib.Path(ROOT, f).read_text(encoding="utf-8"))
    print("boot: postgres schema ensured", flush=True)
else:
    ensure_schema(db)
    print("boot: sqlite schema ensured (EPHEMERAL demo DB - add Postgres for persistence)",
          flush=True)
db.close()
r = subprocess.run([sys.executable, "test/seed_catalog_v2.py", "--if-empty"])
sys.exit(r.returncode)
