const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const os = require('os');
const { getLogger } = require('./logger');

/**
 * License key management system for PixPBO Protect.
 *
 * Key format: PIXPBO-XXXX-XXXX-XXXX-XXXX
 * Keys are validated locally and optionally against a Discord bot server.
 *
 * Key structure encodes:
 * - Expiration date
 * - Feature flags
 * - Checksum
 */
class LicenseManager {
  constructor() {
    this.logger = getLogger();
    this.configDir = this._getConfigDir();
    this.keyFile = path.join(this.configDir, 'license.dat');
    this.activationFile = path.join(this.configDir, 'activation.dat');
    this.SECRET = 'pixpbo-protect-2024-secure-key';
    this.machineId = this._generateMachineId();
  }

  /**
   * Validate a license key
   */
  async validate(key) {
    if (!key || typeof key !== 'string') {
      return { valid: false, reason: 'Chave inválida' };
    }

    key = key.trim().toUpperCase();

    // Check format
    if (!this._isValidFormat(key)) {
      return { valid: false, reason: 'Formato de chave inválido. Use: PIXPBO-XXXX-XXXX-XXXX-XXXX' };
    }

    // Decode and validate
    const decoded = this._decodeKey(key);

    if (!decoded) {
      return { valid: false, reason: 'Chave inválida ou corrompida' };
    }

    // Check expiration
    if (decoded.expiresAt < Date.now()) {
      return {
        valid: false,
        reason: 'Chave expirada',
        expiresAt: new Date(decoded.expiresAt).toLocaleDateString('pt-BR'),
      };
    }

    // Check checksum
    if (!this._verifyChecksum(key)) {
      return { valid: false, reason: 'Checksum inválido' };
    }

    // Check single-use: if key was activated on a different machine, reject
    const activation = this._getActivation(key);
    if (activation && activation.machineId !== this.machineId) {
      return { valid: false, reason: 'Esta chave ja foi ativada em outro computador. Cada chave so pode ser usada uma vez.' };
    }

    return {
      valid: true,
      expiresAt: new Date(decoded.expiresAt).toLocaleDateString('pt-BR'),
      daysRemaining: Math.ceil((decoded.expiresAt - Date.now()) / (1000 * 60 * 60 * 24)),
      features: decoded.features,
      tier: decoded.tier,
    };
  }

  /**
   * Generate a new license key (used by Discord bot)
   */
  static generateKey(durationDays = 30, tier = 'pro') {
    const expiresAt = Date.now() + (durationDays * 24 * 60 * 60 * 1000);
    const features = tier === 'pro' ? 0xFF : 0x0F;

    // Encode expiration into key segments
    const expHex = expiresAt.toString(16).toUpperCase().padStart(12, '0');
    const tierCode = tier === 'pro' ? 'P' : 'B';
    const featHex = features.toString(16).toUpperCase().padStart(2, '0');

    // Generate segments
    const seg1 = expHex.substring(0, 4);
    const seg2 = expHex.substring(4, 8);
    const seg3 = expHex.substring(8, 12);

    // First 2 chars of seg4 encode tier+feature prefix
    const seg4Prefix = `${tierCode}${featHex.charAt(0)}`;

    // Calculate checksum using the same format as _verifyChecksum
    const rawSeg4 = seg4Prefix + 'XX';
    const rawKey = `PIXPBO-${seg1}-${seg2}-${seg3}-${rawSeg4}`;
    const checksum = crypto
      .createHmac('sha256', 'pixpbo-protect-2024-secure-key')
      .update(rawKey)
      .digest('hex')
      .substring(0, 2)
      .toUpperCase();

    const finalSeg4 = seg4Prefix + checksum;
    const key = `PIXPBO-${seg1}-${seg2}-${seg3}-${finalSeg4}`;

    return key;
  }

  /**
   * Save license key to disk (single-use: binds to this machine)
   */
  saveKey(key) {
    if (!fs.existsSync(this.configDir)) {
      fs.mkdirSync(this.configDir, { recursive: true });
    }

    // Check if this key was already activated on a different machine
    const existingActivation = this._getActivation(key);
    if (existingActivation && existingActivation.machineId !== this.machineId) {
      throw new Error('Esta chave ja foi ativada em outro computador. Cada chave so pode ser usada uma vez.');
    }

    // Encrypt key before saving
    const encrypted = this._encrypt(key);
    fs.writeFileSync(this.keyFile, encrypted, 'utf8');

    // Save activation record (binds key to this machine)
    this._saveActivation(key, this.machineId);

    this.logger.info('License key saved and activated on this machine');
  }

  /**
   * Load saved license key
   */
  getKey() {
    if (!fs.existsSync(this.keyFile)) {
      return null;
    }

    try {
      const encrypted = fs.readFileSync(this.keyFile, 'utf8');
      const key = this._decrypt(encrypted);

      // Verify this key is activated on this machine
      const activation = this._getActivation(key);
      if (activation && activation.machineId !== this.machineId) {
        this.logger.error('License key was activated on a different machine');
        return null;
      }

      return key;
    } catch (error) {
      this.logger.error(`Failed to load license: ${error.message}`);
      return null;
    }
  }

  // ---- Internal Methods ----

  _isValidFormat(key) {
    return /^PIXPBO-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$/.test(key);
  }

  _decodeKey(key) {
    try {
      const parts = key.split('-');
      if (parts.length !== 5) return null;

      const expHex = parts[1] + parts[2] + parts[3];
      const expiresAt = parseInt(expHex, 16);

      if (isNaN(expiresAt) || expiresAt < 0) return null;

      const tierChar = parts[4][0];
      const tier = tierChar === 'P' ? 'pro' : 'basic';
      const features = parseInt(parts[4].substring(1, 3), 16);

      return { expiresAt, tier, features };
    } catch (error) {
      return null;
    }
  }

  _verifyChecksum(key) {
    try {
      const parts = key.split('-');
      const lastSeg = parts[4];
      const checkChars = lastSeg.substring(2, 4);

      // Reconstruct key without checksum
      const rawSeg4 = lastSeg.substring(0, 2) + 'XX';
      const rawKey = `PIXPBO-${parts[1]}-${parts[2]}-${parts[3]}-${rawSeg4}`;

      const expectedChecksum = crypto
        .createHmac('sha256', this.SECRET)
        .update(`PIXPBO-${parts[1]}-${parts[2]}-${parts[3]}-${rawSeg4}`)
        .digest('hex')
        .substring(0, 2)
        .toUpperCase();

      return checkChars === expectedChecksum;
    } catch (error) {
      return false;
    }
  }

  /**
   * Generate a unique machine ID based on hardware/OS info
   */
  _generateMachineId() {
    const info = [
      os.hostname(),
      os.platform(),
      os.arch(),
      os.cpus()[0]?.model || 'unknown',
      os.totalmem().toString(),
      os.homedir(),
    ].join('|');

    return crypto
      .createHash('sha256')
      .update(info)
      .digest('hex')
      .substring(0, 16);
  }

  /**
   * Save activation record
   */
  _saveActivation(key, machineId) {
    const data = {
      key: key,
      machineId: machineId,
      activatedAt: new Date().toISOString(),
    };
    const encrypted = this._encrypt(JSON.stringify(data));
    fs.writeFileSync(this.activationFile, encrypted, 'utf8');
  }

  /**
   * Get activation record for a key
   */
  _getActivation(key) {
    if (!fs.existsSync(this.activationFile)) {
      return null;
    }
    try {
      const encrypted = fs.readFileSync(this.activationFile, 'utf8');
      const data = JSON.parse(this._decrypt(encrypted));
      if (data.key === key) {
        return data;
      }
      return null;
    } catch (error) {
      return null;
    }
  }

  static _randomChar() {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    return chars.charAt(Math.floor(Math.random() * chars.length));
  }

  _encrypt(text) {
    const iv = crypto.randomBytes(16);
    const key = crypto.scryptSync(this.SECRET, 'pixpbo-salt', 32);
    const cipher = crypto.createCipheriv('aes-256-cbc', key, iv);
    let encrypted = cipher.update(text, 'utf8', 'hex');
    encrypted += cipher.final('hex');
    return iv.toString('hex') + ':' + encrypted;
  }

  _decrypt(text) {
    const parts = text.split(':');
    const iv = Buffer.from(parts[0], 'hex');
    const key = crypto.scryptSync(this.SECRET, 'pixpbo-salt', 32);
    const decipher = crypto.createDecipheriv('aes-256-cbc', key, iv);
    let decrypted = decipher.update(parts[1], 'hex', 'utf8');
    decrypted += decipher.final('utf8');
    return decrypted;
  }

  _getConfigDir() {
    const appData = process.env.APPDATA ||
      (process.platform === 'darwin'
        ? path.join(process.env.HOME || '', 'Library', 'Application Support')
        : path.join(process.env.HOME || '', '.config'));
    return path.join(appData, 'PixPBO Protect');
  }
}

module.exports = { LicenseManager };
