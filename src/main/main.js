const { app, BrowserWindow, ipcMain, dialog, shell } = require('electron');
const path = require('path');
const fs = require('fs');
const { initLogger, getLogger } = require('../utils/logger');
const { PBOManager } = require('../utils/pbo-manager');
const { Obfuscator } = require('../utils/obfuscator');
const { FileCleaner } = require('../utils/file-cleaner');
const { ModSigner } = require('../utils/mod-signer');
const { ReportGenerator } = require('../utils/report-generator');
const { LicenseManager } = require('../utils/license-manager');
const { BackupManager } = require('../utils/backup-manager');

let mainWindow;
let logger;

// App paths
const APP_PATHS = {
  backups: path.join(app.getPath('userData'), 'Backups'),
  output: path.join(app.getPath('userData'), 'Output'),
  logs: path.join(app.getPath('userData'), 'Logs'),
  keys: path.join(app.getPath('userData'), 'Keys'),
  temp: path.join(app.getPath('userData'), 'Temp'),
};

function ensureDirectories() {
  Object.values(APP_PATHS).forEach(dir => {
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
  });
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 860,
    minWidth: 1024,
    minHeight: 700,
    frame: false,
    transparent: false,
    backgroundColor: '#0a0a1a',
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
    },
    icon: path.join(__dirname, '..', 'assets', 'icon.png'),
    title: 'PixPBO Protect',
  });

  mainWindow.loadFile(path.join(__dirname, '..', 'renderer', 'index.html'));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  ensureDirectories();
  initLogger(APP_PATHS.logs);
  logger = getLogger();
  logger.info('PixPBO Protect started');
  createWindow();
});

app.on('window-all-closed', () => {
  app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// ---- IPC Handlers ----

// Window controls
ipcMain.on('window-minimize', () => mainWindow?.minimize());
ipcMain.on('window-maximize', () => {
  if (mainWindow?.isMaximized()) {
    mainWindow.unmaximize();
  } else {
    mainWindow?.maximize();
  }
});
ipcMain.on('window-close', () => mainWindow?.close());

// Open folder dialog
ipcMain.handle('select-folder', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ['openDirectory'],
    title: 'Selecionar Pasta do Mod DayZ',
  });
  return result.canceled ? null : result.filePaths[0];
});

// Open file dialog for .pbo
ipcMain.handle('select-pbo', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    properties: ['openFile'],
    title: 'Selecionar Arquivo .pbo',
    filters: [{ name: 'PBO Files', extensions: ['pbo'] }],
  });
  return result.canceled ? null : result.filePaths[0];
});

// Analyze PBO
ipcMain.handle('analyze-pbo', async (event, pboPath) => {
  try {
    logger.info(`Analyzing PBO: ${pboPath}`);
    const pboManager = new PBOManager(APP_PATHS);
    const analysis = await pboManager.analyze(pboPath);
    logger.info('PBO analysis complete');
    return { success: true, data: analysis };
  } catch (error) {
    logger.error(`PBO analysis failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Analyze folder
ipcMain.handle('analyze-folder', async (event, folderPath) => {
  try {
    logger.info(`Analyzing folder: ${folderPath}`);
    const pboManager = new PBOManager(APP_PATHS);
    const analysis = await pboManager.analyzeFolder(folderPath);
    logger.info('Folder analysis complete');
    return { success: true, data: analysis };
  } catch (error) {
    logger.error(`Folder analysis failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Obfuscate scripts
ipcMain.handle('obfuscate', async (event, { sourcePath, level }) => {
  try {
    logger.info(`Obfuscating scripts at: ${sourcePath}, level: ${level}`);
    const backupManager = new BackupManager(APP_PATHS.backups);
    await backupManager.createBackup(sourcePath);
    const obfuscator = new Obfuscator(level);
    const result = await obfuscator.processDirectory(sourcePath);
    logger.info('Obfuscation complete');
    return { success: true, data: result };
  } catch (error) {
    logger.error(`Obfuscation failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Clean files
ipcMain.handle('clean-files', async (event, targetPath) => {
  try {
    logger.info(`Cleaning files at: ${targetPath}`);
    const cleaner = new FileCleaner();
    const result = await cleaner.clean(targetPath);
    logger.info('File cleaning complete');
    return { success: true, data: result };
  } catch (error) {
    logger.error(`File cleaning failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Repack PBO
ipcMain.handle('repack-pbo', async (event, { sourcePath, outputPath }) => {
  try {
    logger.info(`Repacking PBO from: ${sourcePath}`);
    const pboManager = new PBOManager(APP_PATHS);
    const outDir = outputPath || APP_PATHS.output;
    const result = await pboManager.repack(sourcePath, outDir);
    logger.info('PBO repacking complete');
    return { success: true, data: result };
  } catch (error) {
    logger.error(`PBO repacking failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Sign mod
ipcMain.handle('sign-mod', async (event, { pboPath, keyName }) => {
  try {
    logger.info(`Signing mod: ${pboPath}`);
    const signer = new ModSigner(APP_PATHS.keys);
    const result = await signer.sign(pboPath, keyName);
    logger.info('Mod signing complete');
    return { success: true, data: result };
  } catch (error) {
    logger.error(`Mod signing failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// Generate report
ipcMain.handle('generate-report', async (event, reportData) => {
  try {
    logger.info('Generating report');
    const generator = new ReportGenerator(APP_PATHS.logs);
    const result = await generator.generate(reportData);
    logger.info('Report generated');
    return { success: true, data: result };
  } catch (error) {
    logger.error(`Report generation failed: ${error.message}`);
    return { success: false, error: error.message };
  }
});

// License validation
ipcMain.handle('validate-license', async (event, key) => {
  try {
    const licenseManager = new LicenseManager();
    const result = await licenseManager.validate(key);
    return { success: true, data: result };
  } catch (error) {
    return { success: false, error: error.message };
  }
});

ipcMain.handle('save-license', async (event, key) => {
  try {
    const licenseManager = new LicenseManager();
    licenseManager.saveKey(key);
    return { success: true };
  } catch (error) {
    return { success: false, error: error.message };
  }
});

ipcMain.handle('get-license', async () => {
  try {
    const licenseManager = new LicenseManager();
    const key = licenseManager.getKey();
    return { success: true, data: key };
  } catch (error) {
    return { success: false, error: error.message };
  }
});

// Get app paths
ipcMain.handle('get-app-paths', async () => {
  return APP_PATHS;
});

// Open path in explorer
ipcMain.handle('open-path', async (event, targetPath) => {
  shell.openPath(targetPath);
});

// Full pipeline
ipcMain.handle('run-pipeline', async (event, { sourcePath, options }) => {
  const results = {
    steps: [],
    errors: [],
    startTime: Date.now(),
  };

  try {
    // Step 1: Backup
    event.sender.send('pipeline-progress', { step: 'backup', status: 'running' });
    const backupManager = new BackupManager(APP_PATHS.backups);
    await backupManager.createBackup(sourcePath);
    results.steps.push({ name: 'Backup', status: 'success' });
    event.sender.send('pipeline-progress', { step: 'backup', status: 'done' });

    // Step 2: Clean
    if (options.clean) {
      event.sender.send('pipeline-progress', { step: 'clean', status: 'running' });
      const cleaner = new FileCleaner();
      const cleanResult = await cleaner.clean(sourcePath);
      results.steps.push({ name: 'Limpeza', status: 'success', data: cleanResult });
      event.sender.send('pipeline-progress', { step: 'clean', status: 'done' });
    }

    // Step 3: Obfuscate
    if (options.obfuscate) {
      event.sender.send('pipeline-progress', { step: 'obfuscate', status: 'running' });
      const obfuscator = new Obfuscator(options.obfuscationLevel || 'medium');
      const obfResult = await obfuscator.processDirectory(sourcePath);
      results.steps.push({ name: 'Obfuscação', status: 'success', data: obfResult });
      event.sender.send('pipeline-progress', { step: 'obfuscate', status: 'done' });
    }

    // Step 4: Repack
    if (options.repack) {
      event.sender.send('pipeline-progress', { step: 'repack', status: 'running' });
      const pboManager = new PBOManager(APP_PATHS);
      const repackResult = await pboManager.repack(sourcePath, APP_PATHS.output);
      results.steps.push({ name: 'Reempacotamento', status: 'success', data: repackResult });
      event.sender.send('pipeline-progress', { step: 'repack', status: 'done' });
    }

    // Step 5: Sign
    if (options.sign) {
      event.sender.send('pipeline-progress', { step: 'sign', status: 'running' });
      const signer = new ModSigner(APP_PATHS.keys);
      const signResult = await signer.sign(
        path.join(APP_PATHS.output, path.basename(sourcePath) + '.pbo'),
        options.keyName || 'pixpbo'
      );
      results.steps.push({ name: 'Assinatura', status: 'success', data: signResult });
      event.sender.send('pipeline-progress', { step: 'sign', status: 'done' });
    }

    // Step 6: Report
    event.sender.send('pipeline-progress', { step: 'report', status: 'running' });
    results.endTime = Date.now();
    const generator = new ReportGenerator(APP_PATHS.logs);
    const report = await generator.generate(results);
    results.steps.push({ name: 'Relatório', status: 'success', data: report });
    event.sender.send('pipeline-progress', { step: 'report', status: 'done' });

    return { success: true, data: results };
  } catch (error) {
    logger.error(`Pipeline failed: ${error.message}`);
    results.errors.push(error.message);
    return { success: false, data: results, error: error.message };
  }
});
