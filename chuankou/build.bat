@echo off
setlocal
cd /d "%~dp0"
if not exist "bin" mkdir "bin"

set "CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"

"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu ^
  /reference:System.dll /reference:System.Core.dll ^
  /reference:System.Drawing.dll /reference:System.Windows.Forms.dll ^
  /win32icon:"Assets\clock-icon.ico" ^
  /out:"bin\SerialTimeSync.exe" Program.cs MainForm.cs

if errorlevel 1 (
  echo.
  echo Build failed.
  pause
  exit /b 1
)

echo.
echo Build succeeded: bin\SerialTimeSync.exe
pause
