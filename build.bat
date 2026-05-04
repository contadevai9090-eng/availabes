@echo off
echo ===================================================
echo   PixPBO Protect - Build Script
echo   Proteja, compacte e assine seus mods DayZ
echo ===================================================
echo.

:: Check Node.js
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERRO] Node.js nao encontrado! Instale em: https://nodejs.org
    pause
    exit /b 1
)

echo [1/4] Verificando Node.js...
node --version

:: Install dependencies
echo.
echo [2/4] Instalando dependencias...
call npm install
if %errorlevel% neq 0 (
    echo [ERRO] Falha ao instalar dependencias!
    pause
    exit /b 1
)

:: Install Discord bot dependencies
echo.
echo [3/4] Instalando dependencias do bot Discord...
cd discord-bot
call npm install
cd ..
if %errorlevel% neq 0 (
    echo [AVISO] Falha ao instalar dependencias do bot (nao critico)
)

:: Build
echo.
echo [4/4] Construindo aplicativo Windows...
call npm run build
if %errorlevel% neq 0 (
    echo [ERRO] Falha no build!
    echo Tentando executar em modo dev...
    echo Use "npm start" para executar em modo desenvolvimento.
    pause
    exit /b 1
)

echo.
echo ===================================================
echo   BUILD COMPLETO!
echo   O instalador esta em: dist\
echo ===================================================
echo.
pause
