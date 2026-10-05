@echo off
cd /d "%~dp0"
set "PYEXE="
set "PYARGS="
where py >nul 2>&1 && set "PYEXE=py" && set "PYARGS=-3"
if not defined PYEXE where python >nul 2>&1 && set "PYEXE=python"
if not defined PYEXE if exist "%LOCALAPPDATA%\Python\pythoncore-3.14-64\python.exe" set "PYEXE=%LOCALAPPDATA%\Python\pythoncore-3.14-64\python.exe"
if not defined PYEXE (
  echo Python bulunamadi.
  pause
  exit /b 1
)
if "%~1"=="" (
  "%PYEXE%" %PYARGS% "%~dp0hdtv_to_gf3.py"
) else (
  "%PYEXE%" %PYARGS% "%~dp0hdtv_to_gf3.py" %*
)
echo.
echo Kaydedildi. Yeni .spe dosyasi girdinin yanindadir.
echo .spe dosyasini python ile calistirmayin ve cift tiklamayin.
pause
