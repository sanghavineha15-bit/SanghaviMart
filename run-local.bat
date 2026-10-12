@echo off
rem SanghaviMart localhost app: serves the REAL storefront + REAL /api/v1
rem on an embedded SQLite database. No Docker, no installs beyond Python,
rem free forever. Picks a free port (8080, else 8091/8092) and opens your browser.
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
python -m pip install --quiet argon2-cffi >nul 2>&1
python test\seed_catalog_v2.py --if-empty
echo Starting SanghaviMart on http://localhost:%APORT% ...
python demo-server.py
pause
