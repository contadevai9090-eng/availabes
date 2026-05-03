const fs = require('fs');
const path = require('path');
const { getLogger } = require('./logger');

/**
 * DayZ Enforce Script (.c) Obfuscator
 *
 * Levels:
 * - light: Remove comments, trim whitespace
 * - medium: + Rename local variables, compact code
 * - strong: + Rename internal functions, aggressive compaction
 *
 * Safety:
 * - Preserves class declarations, modded classes, overrides
 * - Preserves RPC calls, input handlers, config references
 * - Preserves string literals
 * - Preserves public API / DayZ engine calls
 */
class Obfuscator {
  constructor(level = 'medium') {
    this.level = level;
    this.logger = getLogger();
    this.stats = {
      filesProcessed: 0,
      filesSkipped: 0,
      commentsRemoved: 0,
      variablesRenamed: 0,
      functionsRenamed: 0,
      originalSize: 0,
      obfuscatedSize: 0,
    };

    // DayZ reserved keywords and engine classes - NEVER rename these
    this.reserved = new Set([
      // Enforce keywords
      'void', 'int', 'float', 'bool', 'string', 'vector', 'class', 'extends',
      'modded', 'override', 'private', 'protected', 'static', 'const', 'ref',
      'autoptr', 'auto', 'new', 'delete', 'null', 'NULL', 'true', 'false',
      'if', 'else', 'for', 'foreach', 'while', 'do', 'switch', 'case',
      'default', 'break', 'continue', 'return', 'this', 'super', 'typename',
      'typedef', 'enum', 'proto', 'native', 'volatile', 'out', 'inout',
      'owned', 'sealed', 'event', 'array', 'set', 'map',

      // Common DayZ engine classes
      'EntityAI', 'PlayerBase', 'ItemBase', 'DayZPlayer', 'DayZPlayerImplement',
      'ManBase', 'Building', 'Transport', 'CarScript', 'ActionBase',
      'ActionManagerClient', 'ActionManagerServer', 'UIScriptedMenu',
      'ScriptedWidgetEventHandler', 'PluginBase', 'ModuleBase',
      'CfgGameplayHandler', 'Mission', 'MissionBase', 'MissionServer',
      'MissionGameplay', 'Object', 'Weapon_Base', 'Magazine_Base',
      'Clothing_Base', 'Container_Base', 'TentBase', 'BaseBuildingBase',
      'Inventory_Base', 'RecipeBase', 'CraftingManager',

      // Common DayZ methods
      'GetGame', 'GetPlayer', 'GetDayZGame', 'GetMission', 'Print',
      'CF_Log', 'Debug', 'Error', 'OnInit', 'OnUpdate', 'OnDestroy',
      'Init', 'OnRPC', 'OnStoreLoad', 'OnStoreSave', 'OnVariablesSynchronized',
      'GetType', 'GetPosition', 'SetPosition', 'IsInherited', 'IsKindOf',
      'ClassName', 'ToString', 'ToFloat', 'ToInt',

      // RPC/Sync
      'ScriptRPC', 'GetRPCManager', 'RPC_', 'RPCs', 'SyncEvents',
    ]);

    // Patterns that should never be modified
    this.protectedPatterns = [
      /^class\s+\w+/,           // Class declarations
      /modded\s+class/,          // Modded classes
      /override\s+/,             // Override methods
      /\#include/,               // Includes
      /\$CurrentDir/,            // DayZ paths
      /GetRPCManager/,           // RPC calls
      /RegisterAsPluginModule/,  // Plugin registration
    ];
  }

  /**
   * Process all .c files in a directory
   */
  async processDirectory(dirPath) {
    const files = this._findScripts(dirPath);

    for (const filePath of files) {
      try {
        await this._processFile(filePath);
      } catch (error) {
        this.logger.error(`Failed to obfuscate ${filePath}: ${error.message}`);
        this.stats.filesSkipped++;
      }
    }

    return {
      ...this.stats,
      compressionRatio: this.stats.originalSize > 0
        ? ((1 - this.stats.obfuscatedSize / this.stats.originalSize) * 100).toFixed(1) + '%'
        : '0%',
    };
  }

  /**
   * Obfuscate a single script file
   */
  async _processFile(filePath) {
    // Check if file should be skipped (e.g., config files)
    const basename = path.basename(filePath).toLowerCase();
    if (basename === 'config.cpp' || basename === 'config.bin') {
      this.stats.filesSkipped++;
      return;
    }

    const original = fs.readFileSync(filePath, 'utf8');
    this.stats.originalSize += Buffer.byteLength(original, 'utf8');

    let code = original;

    // Level: Light
    code = this._removeComments(code);
    code = this._trimWhitespace(code);

    // Level: Medium
    if (this.level === 'medium' || this.level === 'strong') {
      code = this._renameLocalVariables(code);
      code = this._compactCode(code);
    }

    // Level: Strong
    if (this.level === 'strong') {
      code = this._renameInternalFunctions(code);
      code = this._aggressiveCompact(code);
    }

    fs.writeFileSync(filePath, code, 'utf8');
    this.stats.obfuscatedSize += Buffer.byteLength(code, 'utf8');
    this.stats.filesProcessed++;
  }

  /**
   * Remove all comments (single-line and multi-line)
   */
  _removeComments(code) {
    let result = '';
    let i = 0;
    let inString = false;
    let stringChar = '';
    let commentsRemoved = 0;

    while (i < code.length) {
      // Handle string literals
      if (!inString && (code[i] === '"' || code[i] === "'")) {
        inString = true;
        stringChar = code[i];
        result += code[i];
        i++;
        continue;
      }

      if (inString) {
        if (code[i] === '\\' && i + 1 < code.length) {
          result += code[i] + code[i + 1];
          i += 2;
          continue;
        }
        if (code[i] === stringChar) {
          inString = false;
        }
        result += code[i];
        i++;
        continue;
      }

      // Single-line comment
      if (code[i] === '/' && i + 1 < code.length && code[i + 1] === '/') {
        commentsRemoved++;
        while (i < code.length && code[i] !== '\n') i++;
        continue;
      }

      // Multi-line comment
      if (code[i] === '/' && i + 1 < code.length && code[i + 1] === '*') {
        commentsRemoved++;
        i += 2;
        while (i < code.length && !(code[i] === '*' && i + 1 < code.length && code[i + 1] === '/')) i++;
        i += 2;
        continue;
      }

      result += code[i];
      i++;
    }

    this.stats.commentsRemoved += commentsRemoved;
    return result;
  }

  /**
   * Trim unnecessary whitespace
   */
  _trimWhitespace(code) {
    // Remove trailing whitespace from lines
    code = code.replace(/[ \t]+$/gm, '');
    // Remove multiple empty lines
    code = code.replace(/\n{3,}/g, '\n\n');
    return code;
  }

  /**
   * Rename local variables (variables declared with common types inside functions)
   */
  _renameLocalVariables(code) {
    const lines = code.split('\n');
    const result = [];
    let varCounter = 0;
    let insideFunction = false;
    let braceDepth = 0;
    let localVarMap = {};

    for (let line of lines) {
      const trimmed = line.trim();

      // Track function boundaries
      if (this._isFunctionDeclaration(trimmed)) {
        insideFunction = true;
        braceDepth = 0;
        localVarMap = {};
      }

      // Track braces
      for (const ch of trimmed) {
        if (ch === '{') braceDepth++;
        if (ch === '}') braceDepth--;
      }

      if (insideFunction && braceDepth <= 0 && trimmed.includes('}')) {
        insideFunction = false;
        localVarMap = {};
      }

      // Rename local variables inside functions
      if (insideFunction && braceDepth > 0) {
        // Match local variable declarations: type varName = ...;
        const localVarMatch = trimmed.match(
          /^(int|float|bool|string|vector|auto|ref|autoptr)\s+([a-z_][a-zA-Z0-9_]*)\s*(=|;)/
        );

        if (localVarMatch) {
          const originalName = localVarMatch[2];
          if (!this.reserved.has(originalName) && originalName.length > 2) {
            const obfName = `_v${varCounter++}`;
            localVarMap[originalName] = obfName;
            this.stats.variablesRenamed++;
          }
        }

        // Apply local variable renames
        for (const [original, obfuscated] of Object.entries(localVarMap)) {
          const regex = new RegExp(`\\b${this._escapeRegex(original)}\\b`, 'g');
          // Don't rename in strings
          line = this._replaceOutsideStrings(line, regex, obfuscated);
        }
      }

      result.push(line);
    }

    return result.join('\n');
  }

  /**
   * Rename internal/private functions
   */
  _renameInternalFunctions(code) {
    // Find private/protected function names
    const funcPattern = /(?:private|protected)\s+(?:void|int|float|bool|string|vector|auto|ref)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(/g;
    const funcMap = {};
    let funcCounter = 0;
    let match;

    while ((match = funcPattern.exec(code)) !== null) {
      const funcName = match[1];
      if (!this.reserved.has(funcName) && !funcName.startsWith('On') && !funcName.startsWith('RPC_')) {
        if (!funcMap[funcName]) {
          funcMap[funcName] = `_f${funcCounter++}`;
          this.stats.functionsRenamed++;
        }
      }
    }

    // Apply renames
    for (const [original, obfuscated] of Object.entries(funcMap)) {
      const regex = new RegExp(`\\b${this._escapeRegex(original)}\\b`, 'g');
      code = this._replaceOutsideStrings(code, regex, obfuscated);
    }

    return code;
  }

  /**
   * Compact code - reduce whitespace
   */
  _compactCode(code) {
    const lines = code.split('\n');
    return lines
      .map(line => line.replace(/\t/g, ' ').replace(/ {2,}/g, ' '))
      .filter(line => line.trim() !== '')
      .join('\n');
  }

  /**
   * Aggressive compaction - minimize file size
   */
  _aggressiveCompact(code) {
    const lines = code.split('\n');
    const result = [];

    for (const line of lines) {
      const trimmed = line.trim();
      if (trimmed === '') continue;

      // Preserve preprocessor directives on their own lines
      if (trimmed.startsWith('#')) {
        result.push(trimmed);
        continue;
      }

      result.push(trimmed);
    }

    return result.join('\n');
  }

  // ---- Helper Methods ----

  _findScripts(dirPath) {
    const scripts = [];
    const items = fs.readdirSync(dirPath);
    for (const item of items) {
      const fullPath = path.join(dirPath, item);
      const stat = fs.statSync(fullPath);
      if (stat.isDirectory()) {
        scripts.push(...this._findScripts(fullPath));
      } else if (item.endsWith('.c')) {
        scripts.push(fullPath);
      }
    }
    return scripts;
  }

  _isFunctionDeclaration(line) {
    return /^(?:override\s+)?(?:static\s+)?(?:private\s+|protected\s+|)?(?:void|int|float|bool|string|vector|auto|ref|autoptr)\s+[A-Za-z_]\w*\s*\(/.test(line);
  }

  _escapeRegex(str) {
    return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  }

  _replaceOutsideStrings(line, regex, replacement) {
    // Simple approach: split by strings, replace in non-string parts
    const parts = [];
    let inString = false;
    let stringChar = '';
    let current = '';

    for (let i = 0; i < line.length; i++) {
      if (!inString && (line[i] === '"' || line[i] === "'")) {
        parts.push({ text: current, isString: false });
        current = line[i];
        inString = true;
        stringChar = line[i];
        continue;
      }
      if (inString && line[i] === stringChar && (i === 0 || line[i - 1] !== '\\')) {
        current += line[i];
        parts.push({ text: current, isString: true });
        current = '';
        inString = false;
        continue;
      }
      current += line[i];
    }
    if (current) {
      parts.push({ text: current, isString: inString });
    }

    return parts
      .map(part => part.isString ? part.text : part.text.replace(regex, replacement))
      .join('');
  }
}

module.exports = { Obfuscator };
