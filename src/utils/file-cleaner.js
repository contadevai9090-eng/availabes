const fs = require('fs');
const path = require('path');
const { getLogger } = require('./logger');

/**
 * Removes junk/unnecessary files from mod directories
 */
class FileCleaner {
  constructor() {
    this.logger = getLogger();

    // Files to remove (case-insensitive)
    this.junkFiles = [
      'thumbs.db',
      'desktop.ini',
      '.ds_store',
      '.gitignore',
      '.gitattributes',
    ];

    // Extensions to remove
    this.junkExtensions = [
      '.tmp', '.bak', '.log', '.old', '.orig',
      '.swp', '.swo', '.cache',
    ];

    // Directories to remove
    this.junkDirs = [
      '.git', '.svn', '.vscode', '.idea',
      '__pycache__', 'node_modules', '.hg',
    ];
  }

  /**
   * Clean a directory, removing junk files
   */
  async clean(targetPath) {
    const stats = {
      filesRemoved: [],
      dirsRemoved: [],
      totalFreed: 0,
    };

    await this._cleanRecursive(targetPath, stats);

    return {
      ...stats,
      totalFreedFormatted: this._formatSize(stats.totalFreed),
      filesRemovedCount: stats.filesRemoved.length,
      dirsRemovedCount: stats.dirsRemoved.length,
    };
  }

  async _cleanRecursive(dirPath, stats) {
    if (!fs.existsSync(dirPath)) return;

    const items = fs.readdirSync(dirPath);

    for (const item of items) {
      const fullPath = path.join(dirPath, item);

      try {
        const stat = fs.statSync(fullPath);

        if (stat.isDirectory()) {
          // Check if it's a junk directory
          if (this.junkDirs.includes(item.toLowerCase())) {
            const dirSize = this._getDirSize(fullPath);
            fs.rmSync(fullPath, { recursive: true });
            stats.dirsRemoved.push(fullPath);
            stats.totalFreed += dirSize;
            this.logger.info(`Removed directory: ${fullPath}`);
            continue;
          }

          // Recurse into subdirectories
          await this._cleanRecursive(fullPath, stats);
        } else {
          // Check if it's a junk file
          const nameLower = item.toLowerCase();
          const ext = path.extname(nameLower);

          if (this.junkFiles.includes(nameLower) || this.junkExtensions.includes(ext)) {
            stats.totalFreed += stat.size;
            fs.unlinkSync(fullPath);
            stats.filesRemoved.push(fullPath);
            this.logger.info(`Removed file: ${fullPath}`);
          }
        }
      } catch (error) {
        this.logger.error(`Error cleaning ${fullPath}: ${error.message}`);
      }
    }
  }

  _getDirSize(dirPath) {
    let size = 0;
    try {
      const items = fs.readdirSync(dirPath);
      for (const item of items) {
        const fullPath = path.join(dirPath, item);
        const stat = fs.statSync(fullPath);
        if (stat.isDirectory()) {
          size += this._getDirSize(fullPath);
        } else {
          size += stat.size;
        }
      }
    } catch (e) {
      // ignore
    }
    return size;
  }

  _formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return `${(bytes / Math.pow(1024, i)).toFixed(2)} ${sizes[i]}`;
  }
}

module.exports = { FileCleaner };
