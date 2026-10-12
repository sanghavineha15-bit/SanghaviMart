"""SanghaviMart shared DB layer: PostgreSQL (psycopg) when DATABASE_URL or
POSTGRES_URL is set, otherwise embedded SQLite (stdlib, zero installs).
Same call surface for both: DB().execute(sql_with_%s, params) with
fetchone()/fetchall()/rowcount, plus .transaction() and .close().
SQLite file lives next to this module (sanghavimart.db) and persists
across restarts. No Docker, no installs, free forever."""
import contextlib
import os
import sqlite3

try:
    import psycopg
    _PG_DUP = (psycopg.errors.UniqueViolation,)
except ImportError:  # lite-only machines don't need psycopg at all
    psycopg = None
    _PG_DUP = ()

#: Catch duplicate-key violations on either backend.
DUP_ERRORS = _PG_DUP + (sqlite3.IntegrityError,)

ROOT = os.path.dirname(os.path.abspath(__file__))
LITE_PATH = os.path.join(ROOT, "sanghavimart.db")


class _Tx:
    def __init__(self, db):
        self.db = db

    def __enter__(self):
        if self.db.lite is not None:
            self.db.lite.execute("BEGIN")
        else:
            self.db._pg_tx = self.db.pg.transaction()
            self.db._pg_tx.__enter__()
        return self

    def __exit__(self, *exc):
        if self.db.lite is not None:
            if exc[0] is None:
                self.db.lite.commit()
            else:
                self.db.lite.rollback()
        else:
            self.db._pg_tx.__exit__(*exc)


class DB:
    def __init__(self, dsn=None):
        url = dsn or os.environ.get("DATABASE_URL") or os.environ.get("POSTGRES_URL")
        self.pg = None
        self.lite = None
        if url:
            if psycopg is None:
                raise RuntimeError("psycopg is required for DATABASE_URL (pip install psycopg[binary])")
            self.pg = psycopg.connect(url)
            self.pg.autocommit = True
        else:
            self.lite = sqlite3.connect(LITE_PATH)
            self.lite.isolation_level = None  # true autocommit; transaction() uses explicit BEGIN

    @property
    def is_lite(self):
        return self.lite is not None

    def execute(self, sql, params=()):
        if self.lite is not None:
            sql, params = self._to_lite(sql, params)
            return self.lite.execute(sql, params)
        return self.pg.execute(sql, params)

    @staticmethod
    def _to_lite(sql, params):
        """Translate %s placeholders to ? and ANY(%s)-lists to IN (?,?,...)."""
        if not params:
            return sql.replace("%s", "?"), ()
        import re
        buf, flat, pi = [], [], 0
        for ch in re.split(r"(= ANY\(%s\)|ANY\(%s\)|%s)", sql):
            if ch in ("ANY(%s)", "= ANY(%s)"):
                lst = list(params[pi])
                pi += 1
                buf.append("IN (" + ",".join("?" for _ in lst) + ")")
                flat.extend(lst)
            elif ch == "%s":
                p = params[pi]
                pi += 1
                if isinstance(p, (list, tuple)):
                    buf.append("(" + ",".join("?" for _ in p) + ")")
                    flat.extend(p)
                else:
                    buf.append("?")
                    flat.append(p)
            elif ch:
                buf.append(ch)
        return "".join(buf), tuple(flat)

    def transaction(self):
        return _Tx(self)

    def close(self):
        if self.pg is not None:
            self.pg.close()
        if self.lite is not None:
            self.lite.close()


SCHEMA_FILES = ["db/schema.sql", "db/migrations/V2__add_reviews_table.sql",
                "db/migrations/V3__add_index_orders_status.sql",
                "db/migrations/V5__chat_persistence.sql",
                "db/migrations/V6__login_codes.sql"]


def ensure_schema(db):
    """Create embedded tables on first boot (Postgres is migrated by V1..Vn)."""
    if not db.is_lite:
        return
    has = db.execute(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='users'").fetchone()[0]
    if has:
        # ensure newer tables exist even on old files
        for f in SCHEMA_FILES[1:]:
            try:
                db.lite.executescript(open(os.path.join(ROOT, f), encoding="utf-8").read())
            except Exception:
                pass
        return
    db.lite.executescript(open(os.path.join(ROOT, SCHEMA_FILES[0]), encoding="utf-8").read())
    for f in SCHEMA_FILES[1:]:
        try:
            db.lite.executescript(open(os.path.join(ROOT, f), encoding="utf-8").read())
        except Exception:
            pass
