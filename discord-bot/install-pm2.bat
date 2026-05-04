@echo off
echo ===================================================
echo   PixPBO Protect - Instalacao PM2 (Bot 24/7)
echo ===================================================
echo.

:: Check Node.js
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERRO] Node.js nao encontrado! Instale em: https://nodejs.org
    pause
    exit /b 1
)

echo [1/3] Instalando PM2 globalmente...
call npm install -g pm2
if %errorlevel% neq 0 (
    echo [ERRO] Falha ao instalar PM2
    pause
    exit /b 1
)

echo.
echo [2/3] Instalando dependencias do bot...
call npm install
if %errorlevel% neq 0 (
    echo [ERRO] Falha ao instalar dependencias
    pause
    exit /b 1
)

echo.
echo [3/3] Iniciando bot com PM2...
call pm2 start bot.js --name "pixpbo-bot"
call pm2 save

echo.
echo ===================================================
echo   Bot iniciado com PM2! 
echo   Comandos uteis:
echo     pm2 status          - Ver status
echo     pm2 logs pixpbo-bot - Ver logs
echo     pm2 restart pixpbo-bot - Reiniciar
echo     pm2 stop pixpbo-bot - Parar
echo ===================================================
echo.
pause
