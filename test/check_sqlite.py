import sqlite3
con = sqlite3.connect(":memory:")
con.executescript(open("F:/ayyapacapc/sanghavimart/db/schema.sql", encoding="utf-8").read())
tables = con.execute("select name from sqlite_master where type='table' order by 1").fetchall()
print("sqlite-schema-ok", tables)
con.execute("INSERT INTO users(name,email,password_hash,role) VALUES(?,?,?,?)",
            ("A", "a@x.local", "h", "BUYER"))
try:
    con.execute("INSERT INTO users(name,email,password_hash,role) VALUES(?,?,?,?)",
                ("B", "a@x.local", "h", "BUYER"))
    print("sqlite-UNIQUE-FAIL")
except sqlite3.IntegrityError:
    print("sqlite-unique-ok")
