@echo off
echo ===================================================
echo   PixPBO Protect - Discord Bot
echo   Sistema de Licencas 24/7
echo ===================================================
echo.

:: Check Node.js
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERRO] Node.js nao encontrado! Instale em: https://nodejs.org
    pause
    exit /b 1
)

:: Install dependencies if needed
if not exist "node_modules" (
    echo Instalando dependencias...
    call npm install
    echo.
)

echo Iniciando bot...
echo Para parar o bot, feche esta janela ou pressione Ctrl+C
echo.
node bot.js
pause
