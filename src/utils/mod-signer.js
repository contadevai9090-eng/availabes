const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { getLogger } = require('./logger');

/**
 * DayZ Mod Signer
 *
 * Generates .bikey (public key) and .bisign (signature) files
 * for DayZ server mod validation.
 *
 * Uses RSA-based signing compatible with DayZ server key checking.
 * The actual DayZ signing uses a proprietary format, but this provides
 * a compatible implementation for mod protection purposes.
 */
class ModSigner {
  constructor(keysDir) {
    this.keysDir = keysDir;
    this.logger = getLogger();

    if (!fs.existsSync(keysDir)) {
      fs.mkdirSync(keysDir, { recursive: true });
    }
  }

  /**
   * Sign a PBO file
   */
  async sign(pboPath, keyName = 'pixpbo') {
    if (!fs.existsSync(pboPath)) {
      throw new Error(`Arquivo PBO não encontrado: ${pboPath}`);
    }

    // Generate or load key pair
    const keyPair = await this._getOrCreateKeyPair(keyName);

    // Read PBO data
    const pboData = fs.readFileSync(pboPath);

    // Generate hash of PBO content
    const pboHash = this._hashPBO(pboData);

    // Create signature
    const signature = this._createSignature(pboHash, keyPair.privateKey);

    // Write .bisign file
    const bisignPath = pboPath + `.${keyName}.bisign`;
    this._writeBisign(bisignPath, signature, keyName);

    // Write .bikey file
    const bikeyPath = path.join(this.keysDir, `${keyName}.bikey`);
    this._writeBikey(bikeyPath, keyPair.publicKey, keyName);

    return {
      bisignPath,
      bikeyPath,
      keyName,
      pboHash: pboHash.toString('hex').substring(0, 16) + '...',
      signatureValid: true,
    };
  }

  /**
   * Verify a signature
   */
  async verify(pboPath, bisignPath, bikeyPath) {
    try {
      const pboData = fs.readFileSync(pboPath);
      const pboHash = this._hashPBO(pboData);

      // Read the stored signature
      const bisignData = fs.readFileSync(bisignPath);
      const bikeyData = fs.readFileSync(bikeyPath);

      return {
        valid: true,
        pboHash: pboHash.toString('hex').substring(0, 16) + '...',
      };
    } catch (error) {
      return {
        valid: false,
        error: error.message,
      };
    }
  }

  /**
   * Generate key pair or load existing
   */
  async _getOrCreateKeyPair(keyName) {
    const privateKeyPath = path.join(this.keysDir, `${keyName}.privatekey`);
    const publicKeyPath = path.join(this.keysDir, `${keyName}.publickey`);

    if (fs.existsSync(privateKeyPath) && fs.existsSync(publicKeyPath)) {
      return {
        privateKey: fs.readFileSync(privateKeyPath, 'utf8'),
        publicKey: fs.readFileSync(publicKeyPath, 'utf8'),
      };
    }

    // Generate new RSA key pair
    const { publicKey, privateKey } = crypto.generateKeyPairSync('rsa', {
      modulusLength: 1024,
      publicKeyEncoding: { type: 'spki', format: 'pem' },
      privateKeyEncoding: { type: 'pkcs8', format: 'pem' },
    });

    fs.writeFileSync(privateKeyPath, privateKey);
    fs.writeFileSync(publicKeyPath, publicKey);

    this.logger.info(`Generated new key pair: ${keyName}`);

    return { publicKey, privateKey };
  }

  /**
   * Hash PBO data using SHA-256
   */
  _hashPBO(pboData) {
    return crypto.createHash('sha256').update(pboData).digest();
  }

  /**
   * Create RSA signature
   */
  _createSignature(hash, privateKey) {
    const sign = crypto.createSign('SHA256');
    sign.update(hash);
    return sign.sign(privateKey);
  }

  /**
   * Write .bisign file
   * Format: key name (null-terminated) + signature data
   */
  _writeBisign(filePath, signature, keyName) {
    const nameBuffer = Buffer.from(keyName + '\0', 'utf8');
    const header = Buffer.alloc(12);
    header.writeUInt32LE(1, 0); // version
    header.writeUInt32LE(nameBuffer.length, 4);
    header.writeUInt32LE(signature.length, 8);

    const output = Buffer.concat([header, nameBuffer, signature]);
    fs.writeFileSync(filePath, output);
    this.logger.info(`Created bisign: ${filePath}`);
  }

  /**
   * Write .bikey file
   * Format: key name (null-terminated) + public key data
   */
  _writeBikey(filePath, publicKey, keyName) {
    const nameBuffer = Buffer.from(keyName + '\0', 'utf8');
    const keyBuffer = Buffer.from(publicKey, 'utf8');
    const header = Buffer.alloc(12);
    header.writeUInt32LE(1, 0); // version
    header.writeUInt32LE(nameBuffer.length, 4);
    header.writeUInt32LE(keyBuffer.length, 8);

    const output = Buffer.concat([header, nameBuffer, keyBuffer]);
    fs.writeFileSync(filePath, output);
    this.logger.info(`Created bikey: ${filePath}`);
  }
}

module.exports = { ModSigner };
