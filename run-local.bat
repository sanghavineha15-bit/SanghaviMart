@echo off
rem SanghaviMart localhost app: boots Postgres, serves the REAL storefront +
rem REAL /api/v1 (picks a free port: 8080, else 8091/8092) and opens your browser.
rem Demo backend is Python (same contract + same DB as the C++ binary).
rem Usage: run-local.bat   (expects docker + python on PATH)
setlocal
cd /d %~dp0
echo Booting DB...
docker start sanghavimart_db >nul 2>&1
if errorlevel 1 docker run -d --name sanghavimart_db -e POSTGRES_PASSWORD=sanghavi_pw -e POSTGRES_USER=sanghavi -e POSTGRES_DB=sanghavimart -p 5434:5432 postgres:15-alpine >nul
for /L %%i in (1,1,30) do (
  docker exec sanghavimart_db pg_isready -U sanghavi >nul 2>&1
  if not errorlevel 1 goto ready
  timeout /t 2 /nobreak >nul
)
echo DB not ready - is Docker Desktop running?
pause
exit /b 1
:ready
python -m pip install --quiet psycopg argon2-cffi >nul 2>&1
set APORT=8080
netstat -ano | findstr /R /C:":8080 .*LISTENING" >nul
if not errorlevel 1 set APORT=8091
if "%APORT%"=="8091" (
  netstat -ano | findstr /R /C:":8091 .*LISTENING" >nul
  if not errorlevel 1 set APORT=8092
)
set PORT=%APORT%
python test\seed_catalog_v2.py --if-empty
echo Starting SanghaviMart on http://localhost:%APORT% ...
python demo-server.py
pause
