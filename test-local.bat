@echo off
rem SanghaviMart full localhost test, no Docker needed.
rem Postgres suite runs only if a PG is listening on :5434, otherwise SKIP.
rem Everything else (catalog reseed on SQLite, JWT, SQLite, C++ core) always runs.
rem Usage: test-local.bat  (expects python + g++ on PATH)
setlocal
cd /d %~dp0
echo [1/5] Environment...
python -m pip install --quiet argon2-cffi >nul 2>&1
echo [2/5] Postgres suite...
python -c "import psycopg; psycopg.connect(host='127.0.0.1',port=5434,dbname='sanghavimart',user='sanghavi',password='sanghavi_pw',connect_timeout=5).close()" >nul 2>&1
if errorlevel 1 ( echo SKIP - no Postgres on :5434, PG suite needs Docker & goto pgdone )
python test\dynamic_pg.py || goto fail
:pgdone
echo [3/5] Catalog reseed (SQLite, embedded)...
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
