const winston = require('winston');
const path = require('path');

let logger = null;

function initLogger(logDir) {
  logger = winston.createLogger({
    level: 'info',
    format: winston.format.combine(
      winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
      winston.format.printf(({ timestamp, level, message }) => {
        return `[${timestamp}] [${level.toUpperCase()}] ${message}`;
      })
    ),
    transports: [
      new winston.transports.File({
        filename: path.join(logDir, 'pixpbo-error.log'),
        level: 'error',
        maxsize: 5 * 1024 * 1024,
        maxFiles: 3,
      }),
      new winston.transports.File({
        filename: path.join(logDir, 'pixpbo.log'),
        maxsize: 10 * 1024 * 1024,
        maxFiles: 5,
      }),
      new winston.transports.Console({
        format: winston.format.combine(
          winston.format.colorize(),
          winston.format.simple()
        ),
      }),
    ],
  });
}

function getLogger() {
  if (!logger) {
    // Fallback console logger
    return {
      info: (msg) => console.log(`[INFO] ${msg}`),
      warn: (msg) => console.warn(`[WARN] ${msg}`),
      error: (msg) => console.error(`[ERROR] ${msg}`),
      debug: (msg) => console.debug(`[DEBUG] ${msg}`),
    };
  }
  return logger;
}

module.exports = { initLogger, getLogger };
