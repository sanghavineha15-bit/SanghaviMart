@echo off
rem SanghaviMart full localhost test: DB boot + Postgres suite + JWT + SQLite + C++ core.
rem Usage: test-local.bat  (expects docker + python + g++ on PATH)
setlocal
cd /d %~dp0
echo [1/4] DB...
docker start sanghavimart_db >nul 2>&1
if errorlevel 1 docker run -d --name sanghavimart_db -e POSTGRES_PASSWORD=sanghavi_pw -e POSTGRES_USER=sanghavi -e POSTGRES_DB=sanghavimart -p 5434:5432 postgres:15-alpine >nul
for /L %%i in (1,1,30) do (
  docker exec sanghavimart_db pg_isready -U sanghavi >nul 2>&1
  if not errorlevel 1 goto ready
  timeout /t 2 /nobreak >nul
)
echo DB not ready - is Docker Desktop running?
goto fail
:ready
echo [2/5] Postgres suite...
python test\dynamic_pg.py || goto fail
echo [3/5] Catalog reseed (tests reset the DB)...
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
