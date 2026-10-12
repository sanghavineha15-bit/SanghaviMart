@echo off
rem SanghaviMart localhost app: native Windows PostgreSQL (no Docker) + REAL
rem storefront + REAL /api/v1. Picks a free port (8080, else 8091/8092) and
rem opens your browser. PostgreSQL 18 service must be installed+running.
rem Usage: run-local.bat   (expects python on PATH)
setlocal
cd /d %~dp0
set APORT=8080
netstat -ano | findstr /R /C:":8080 .*LISTENING" >nul
if not errorlevel 1 set APORT=8091
if "%APORT%"=="8091" (
  netstat -ano | findstr /R /C:":8091 .*LISTENING" >nul
  if not errorlevel 1 set APORT=8092
)
set PORT=%APORT%
set DATABASE_URL=host=127.0.0.1 port=5433 dbname=sanghavimart user=sanghavi password=sanghavi_pw
python -m pip install --quiet argon2-cffi psycopg >nul 2>&1
python test\setup_pg.py || goto fail
python test\seed_catalog_v2.py --if-empty
echo Starting SanghaviMart on http://localhost:%APORT% ...
python demo-server.py
pause
exit /b 0
:fail
echo FAILED - see messages above.
pause
exit /b 1
