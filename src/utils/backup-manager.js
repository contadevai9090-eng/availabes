const fs = require('fs');
const path = require('path');
const AdmZip = require('adm-zip');
const { getLogger } = require('./logger');

/**
 * Manages backups of mod files before processing
 */
class BackupManager {
  constructor(backupsDir) {
    this.backupsDir = backupsDir;
    this.logger = getLogger();

    if (!fs.existsSync(backupsDir)) {
      fs.mkdirSync(backupsDir, { recursive: true });
    }
  }

  /**
   * Create a zip backup of a file or directory
   */
  async createBackup(sourcePath) {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const baseName = path.basename(sourcePath);
    const backupName = `${baseName}_backup_${timestamp}.zip`;
    const backupPath = path.join(this.backupsDir, backupName);

    const zip = new AdmZip();

    const stat = fs.statSync(sourcePath);
    if (stat.isDirectory()) {
      zip.addLocalFolder(sourcePath, baseName);
    } else {
      zip.addLocalFile(sourcePath);
    }

    zip.writeZip(backupPath);
    this.logger.info(`Backup created: ${backupPath}`);

    // Clean old backups (keep last 10)
    await this._cleanOldBackups(10);

    return {
      backupPath,
      backupSize: fs.statSync(backupPath).size,
      backupSizeFormatted: this._formatSize(fs.statSync(backupPath).size),
      timestamp: new Date().toISOString(),
    };
  }

  /**
   * List all backups
   */
  listBackups() {
    if (!fs.existsSync(this.backupsDir)) return [];

    return fs.readdirSync(this.backupsDir)
      .filter(f => f.endsWith('.zip'))
      .map(f => {
        const fullPath = path.join(this.backupsDir, f);
        const stat = fs.statSync(fullPath);
        return {
          name: f,
          path: fullPath,
          size: stat.size,
          sizeFormatted: this._formatSize(stat.size),
          created: stat.birthtime,
        };
      })
      .sort((a, b) => b.created - a.created);
  }

  /**
   * Restore from a backup
   */
  async restore(backupPath, targetDir) {
    if (!fs.existsSync(backupPath)) {
      throw new Error(`Backup não encontrado: ${backupPath}`);
    }

    const zip = new AdmZip(backupPath);
    zip.extractAllTo(targetDir, true);
    this.logger.info(`Backup restored from ${backupPath} to ${targetDir}`);

    return { restored: true, targetDir };
  }

  async _cleanOldBackups(keepCount) {
    const backups = this.listBackups();
    if (backups.length > keepCount) {
      const toDelete = backups.slice(keepCount);
      for (const backup of toDelete) {
        try {
          fs.unlinkSync(backup.path);
          this.logger.info(`Old backup removed: ${backup.name}`);
        } catch (e) {
          // ignore
        }
      }
    }
  }

  _formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return `${(bytes / Math.pow(1024, i)).toFixed(2)} ${sizes[i]}`;
  }
}

module.exports = { BackupManager };
