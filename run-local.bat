@echo off
rem SanghaviMart localhost app: boots Postgres, serves the REAL storefront +
rem REAL /api/v1 on http://localhost:8080 and opens your browser.
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
echo Starting SanghaviMart on http://localhost:8080 ...
python demo-server.py
pause
