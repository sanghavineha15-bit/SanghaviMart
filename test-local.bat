@echo off
rem SanghaviMart full localhost test, no Docker needed.
rem Uses native Windows PostgreSQL on :5433 (setup_pg.py ensures role+db).
rem Usage: test-local.bat  (expects python + g++ on PATH)
setlocal
cd /d %~dp0
echo [1/5] Environment...
set DATABASE_URL=host=127.0.0.1 port=5433 dbname=sanghavimart user=sanghavi password=sanghavi_pw
python -m pip install --quiet argon2-cffi psycopg >nul 2>&1
python test\setup_pg.py || goto fail
echo [2/5] Postgres suite...
python test\dynamic_pg.py || goto fail
echo [3/5] Catalog reseed...
python test\seed_catalog_v2.py || goto fail
echo [4/5] JWT + SQLite...
python test\jwt_contract.py || goto fail
python test\check_sqlite.py || goto fail
echo [5/5] C++ core...
g++ -std=c++20 -fsyntax-only -Wall -Wextra -Isrc src\model\Entities.cc || goto fail
echo ALL-LOCAL-GREEN
pause
exit /b 0
:fail
echo FAILED - see messages above.
pause
exit /b 1
