const fs = require('fs');
const path = require('path');
const { getLogger } = require('./logger');

/**
 * PBO file format manager.
 * Handles analysis, extraction, and repacking of DayZ .pbo files.
 *
 * PBO binary format:
 * - Header entries (filename, packing method, original size, reserved, timestamp, data size)
 * - Empty entry (marks end of header)
 * - File data blocks
 */
class PBOManager {
  constructor(appPaths) {
    this.appPaths = appPaths;
    this.logger = getLogger();
  }

  /**
   * Analyze a .pbo file and return its structure
   */
  async analyze(pboPath) {
    if (!fs.existsSync(pboPath)) {
      throw new Error(`Arquivo PBO não encontrado: ${pboPath}`);
    }

    const stat = fs.statSync(pboPath);
    const buffer = fs.readFileSync(pboPath);
    const entries = this._readPBOHeader(buffer);

    const analysis = {
      filePath: pboPath,
      fileName: path.basename(pboPath),
      fileSize: stat.size,
      fileSizeFormatted: this._formatSize(stat.size),
      totalFiles: entries.length,
      entries: entries,
      scripts: entries.filter(e => e.filename.endsWith('.c')),
      configs: entries.filter(e =>
        e.filename === 'config.cpp' ||
        e.filename === 'config.bin' ||
        e.filename.endsWith('.cpp') ||
        e.filename.endsWith('.hpp')
      ),
      textures: entries.filter(e =>
        e.filename.endsWith('.paa') ||
        e.filename.endsWith('.pac')
      ),
      models: entries.filter(e => e.filename.endsWith('.p3d')),
      sounds: entries.filter(e =>
        e.filename.endsWith('.ogg') ||
        e.filename.endsWith('.wss')
      ),
      other: entries.filter(e =>
        !e.filename.endsWith('.c') &&
        !e.filename.endsWith('.cpp') &&
        !e.filename.endsWith('.hpp') &&
        !e.filename.endsWith('.paa') &&
        !e.filename.endsWith('.pac') &&
        !e.filename.endsWith('.p3d') &&
        !e.filename.endsWith('.ogg') &&
        !e.filename.endsWith('.wss') &&
        e.filename !== 'config.bin'
      ),
      warnings: [],
    };

    // Check for potential issues
    if (analysis.scripts.length === 0) {
      analysis.warnings.push('Nenhum script .c encontrado no PBO');
    }
    if (analysis.configs.length === 0) {
      analysis.warnings.push('Nenhum config.cpp/config.bin encontrado');
    }

    return analysis;
  }

  /**
   * Analyze a mod folder
   */
  async analyzeFolder(folderPath) {
    if (!fs.existsSync(folderPath)) {
      throw new Error(`Pasta não encontrada: ${folderPath}`);
    }

    const files = this._walkDir(folderPath);
    const relativePaths = files.map(f => ({
      filename: path.relative(folderPath, f),
      fullPath: f,
      size: fs.statSync(f).size,
    }));

    const totalSize = relativePaths.reduce((sum, f) => sum + f.size, 0);

    const analysis = {
      folderPath: folderPath,
      folderName: path.basename(folderPath),
      totalSize: totalSize,
      totalSizeFormatted: this._formatSize(totalSize),
      totalFiles: relativePaths.length,
      entries: relativePaths,
      scripts: relativePaths.filter(e => e.filename.endsWith('.c')),
      configs: relativePaths.filter(e =>
        e.filename === 'config.cpp' ||
        e.filename === 'config.bin' ||
        e.filename.endsWith('.cpp') ||
        e.filename.endsWith('.hpp')
      ),
      textures: relativePaths.filter(e =>
        e.filename.endsWith('.paa') ||
        e.filename.endsWith('.pac')
      ),
      models: relativePaths.filter(e => e.filename.endsWith('.p3d')),
      pbos: relativePaths.filter(e => e.filename.endsWith('.pbo')),
      junk: relativePaths.filter(e => {
        const name = path.basename(e.filename).toLowerCase();
        return name === 'thumbs.db' ||
               name === 'desktop.ini' ||
               name === '.ds_store' ||
               name.endsWith('.tmp') ||
               name.endsWith('.bak') ||
               name.endsWith('.log');
      }),
      warnings: [],
    };

    if (analysis.junk.length > 0) {
      analysis.warnings.push(`${analysis.junk.length} arquivo(s) inútil(eis) encontrado(s)`);
    }

    return analysis;
  }

  /**
   * Extract PBO contents to a temporary directory
   */
  async extract(pboPath) {
    const buffer = fs.readFileSync(pboPath);
    const entries = this._readPBOHeader(buffer);
    const extractDir = path.join(this.appPaths.temp, path.basename(pboPath, '.pbo'));

    if (fs.existsSync(extractDir)) {
      fs.rmSync(extractDir, { recursive: true });
    }
    fs.mkdirSync(extractDir, { recursive: true });

    let dataOffset = this._getDataOffset(buffer);

    for (const entry of entries) {
      if (entry.dataSize > 0) {
        const filePath = path.join(extractDir, entry.filename);
        const fileDir = path.dirname(filePath);
        if (!fs.existsSync(fileDir)) {
          fs.mkdirSync(fileDir, { recursive: true });
        }
        const fileData = buffer.slice(dataOffset, dataOffset + entry.dataSize);
        fs.writeFileSync(filePath, fileData);
        dataOffset += entry.dataSize;
      }
    }

    return extractDir;
  }

  /**
   * Repack a folder into a .pbo file
   */
  async repack(sourcePath, outputDir) {
    const modName = path.basename(sourcePath);
    const outputPath = path.join(outputDir, `${modName}.pbo`);
    const files = this._walkDir(sourcePath);

    // Build PBO
    const entries = [];
    const fileBuffers = [];

    for (const filePath of files) {
      const relativePath = path.relative(sourcePath, filePath).replace(/\//g, '\\');
      const fileBuffer = fs.readFileSync(filePath);

      entries.push({
        filename: relativePath,
        packingMethod: 0,
        originalSize: fileBuffer.length,
        reserved: 0,
        timestamp: Math.floor(Date.now() / 1000),
        dataSize: fileBuffer.length,
      });

      fileBuffers.push(fileBuffer);
    }

    // Write PBO
    const headerBuffers = [];

    // Write file entries
    for (const entry of entries) {
      headerBuffers.push(this._writeHeaderEntry(entry));
    }

    // Write empty entry (end of header)
    headerBuffers.push(this._writeHeaderEntry({
      filename: '',
      packingMethod: 0,
      originalSize: 0,
      reserved: 0,
      timestamp: 0,
      dataSize: 0,
    }));

    const headerBuffer = Buffer.concat(headerBuffers);
    const dataBuffer = Buffer.concat(fileBuffers);
    const pboBuffer = Buffer.concat([headerBuffer, dataBuffer]);

    fs.writeFileSync(outputPath, pboBuffer);

    const originalSize = files.reduce((sum, f) => sum + fs.statSync(f).size, 0);

    return {
      outputPath: outputPath,
      originalSize: originalSize,
      packedSize: pboBuffer.length,
      originalSizeFormatted: this._formatSize(originalSize),
      packedSizeFormatted: this._formatSize(pboBuffer.length),
      filesIncluded: entries.length,
    };
  }

  // ---- Internal Methods ----

  _readPBOHeader(buffer) {
    const entries = [];
    let offset = 0;

    while (offset < buffer.length) {
      // Read null-terminated filename
      let filenameEnd = offset;
      while (filenameEnd < buffer.length && buffer[filenameEnd] !== 0) {
        filenameEnd++;
      }

      const filename = buffer.toString('utf8', offset, filenameEnd);
      offset = filenameEnd + 1;

      if (offset + 20 > buffer.length) break;

      const packingMethod = buffer.readUInt32LE(offset); offset += 4;
      const originalSize = buffer.readUInt32LE(offset); offset += 4;
      const reserved = buffer.readUInt32LE(offset); offset += 4;
      const timestamp = buffer.readUInt32LE(offset); offset += 4;
      const dataSize = buffer.readUInt32LE(offset); offset += 4;

      // Empty filename marks end of header
      if (filename === '') {
        // Check for prefix/product entry (Vers/sreV)
        if (packingMethod === 0x56657273 || packingMethod === 0x73726556) {
          // Skip header extensions
          while (offset < buffer.length && buffer[offset] !== 0) {
            while (offset < buffer.length && buffer[offset] !== 0) offset++;
            offset++; // skip null
            while (offset < buffer.length && buffer[offset] !== 0) offset++;
            offset++; // skip null
          }
          offset++; // skip final null
          continue;
        }
        break;
      }

      entries.push({
        filename: filename.replace(/\\/g, '/'),
        packingMethod,
        originalSize,
        reserved,
        timestamp,
        dataSize: dataSize || originalSize,
        sizeFormatted: this._formatSize(dataSize || originalSize),
      });
    }

    this._headerEndOffset = offset;
    return entries;
  }

  _getDataOffset(buffer) {
    this._readPBOHeader(buffer);
    return this._headerEndOffset || 0;
  }

  _writeHeaderEntry(entry) {
    const filenameBuffer = Buffer.from(entry.filename + '\0', 'utf8');
    const metaBuffer = Buffer.alloc(20);
    metaBuffer.writeUInt32LE(entry.packingMethod, 0);
    metaBuffer.writeUInt32LE(entry.originalSize, 4);
    metaBuffer.writeUInt32LE(entry.reserved, 8);
    metaBuffer.writeUInt32LE(entry.timestamp, 12);
    metaBuffer.writeUInt32LE(entry.dataSize, 16);
    return Buffer.concat([filenameBuffer, metaBuffer]);
  }

  _walkDir(dirPath) {
    const files = [];
    const items = fs.readdirSync(dirPath);
    for (const item of items) {
      const fullPath = path.join(dirPath, item);
      const stat = fs.statSync(fullPath);
      if (stat.isDirectory()) {
        files.push(...this._walkDir(fullPath));
      } else {
        files.push(fullPath);
      }
    }
    return files;
  }

  _formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return `${(bytes / Math.pow(1024, i)).toFixed(2)} ${sizes[i]}`;
  }
}

module.exports = { PBOManager };
