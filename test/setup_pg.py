"""Local PostgreSQL bootstrap (no Docker): uses the native Windows PostgreSQL
service on 127.0.0.1:5433, ensures the sanghavi role + sanghavimart database.
Idempotent. Used by run-local.bat and test-local.bat."""
import sys

try:
    import psycopg
except ImportError:
    sys.exit("FATAL: pip install psycopg[binary]")

HOST, PORT, DB, USER, PW = "127.0.0.1", 5433, "sanghavimart", "sanghavi", "sanghavi_pw"
SUPER = {"host": HOST, "port": PORT, "dbname": "postgres",
         "user": "postgres", "password": "postgres", "connect_timeout": 10}

try:
    a = psycopg.connect(**SUPER)
except Exception as e:  # noqa: BLE001
    sys.exit(f"FATAL: native PostgreSQL not reachable on {HOST}:{PORT}: {e}")
a.autocommit = True
a.execute("DO $$ BEGIN IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname='sanghavi') "
          "THEN CREATE ROLE sanghavi LOGIN PASSWORD 'sanghavi_pw'; ELSE "
          "ALTER ROLE sanghavi WITH LOGIN PASSWORD 'sanghavi_pw'; END IF; END $$")
r = a.execute("SELECT 1 FROM pg_database WHERE datname=%s", (DB,)).fetchone()
if not r:
    a.execute(f"CREATE DATABASE {DB} OWNER {USER}")
a.execute(f"GRANT ALL PRIVILEGES ON DATABASE {DB} TO {USER}")
b = psycopg.connect(host=HOST, port=PORT, dbname=DB, user=USER, password=PW, connect_timeout=10)
b.autocommit = True
b.execute("GRANT ALL ON SCHEMA public TO sanghavi")
b.execute("ALTER SCHEMA public OWNER TO sanghavi")
b.close()
a.close()
print(f"postgres ready: {HOST}:{PORT}/{DB} as {USER}")
