// =====================================================
// PixPBO Protect - Renderer App Logic
// =====================================================

let currentPath = null;
let lastReport = null;

// ---- Tab Navigation ----
function switchTab(tabName) {
  // Hide all tabs
  document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
  document.querySelectorAll('.nav-btn').forEach(el => el.classList.remove('active'));

  // Show selected tab
  const tab = document.getElementById(`tab-${tabName}`);
  if (tab) tab.classList.add('active');

  const btn = document.querySelector(`[data-tab="${tabName}"]`);
  if (btn) btn.classList.add('active');
}

// ---- License Management ----
async function checkSavedLicense() {
  try {
    const result = await window.pixpbo.getLicense();
    if (result.success && result.data) {
      const validation = await window.pixpbo.validateLicense(result.data);
      if (validation.success && validation.data.valid) {
        showApp(validation.data);
        return;
      }
    }
  } catch (e) {
    // No saved license
  }
}

async function validateLicense() {
  const input = document.getElementById('license-input');
  const errorEl = document.getElementById('license-error');
  const infoEl = document.getElementById('license-info');
  const key = input.value.trim().toUpperCase();

  errorEl.textContent = '';
  infoEl.textContent = '';

  if (!key) {
    errorEl.textContent = 'Por favor, insira uma chave de licenca.';
    return;
  }

  try {
    showLoading('Validando licenca...');
    const result = await window.pixpbo.validateLicense(key);
    hideLoading();

    if (result.success && result.data.valid) {
      // Save the key
      await window.pixpbo.saveLicense(key);
      infoEl.textContent = `Licenca ativa! Expira em: ${result.data.expiresAt} (${result.data.daysRemaining} dias)`;
      setTimeout(() => showApp(result.data), 1000);
    } else {
      errorEl.textContent = result.data?.reason || result.error || 'Chave invalida';
    }
  } catch (error) {
    hideLoading();
    errorEl.textContent = 'Erro ao validar licenca: ' + error.message;
  }
}

function showApp(licenseData) {
  document.getElementById('license-gate').classList.add('hidden');
  document.getElementById('main-app').classList.remove('hidden');

  if (licenseData) {
    document.getElementById('license-status-text').textContent =
      `${licenseData.tier === 'pro' ? 'Pro' : 'Basic'} - ${licenseData.daysRemaining}d`;
  }
}

// ---- Analyze ----
async function selectAndAnalyzePBO() {
  const pboPath = await window.pixpbo.selectPBO();
  if (!pboPath) return;

  currentPath = pboPath;
  showPath('analyze-path', pboPath);
  showLoading('Analisando PBO...');

  const result = await window.pixpbo.analyzePBO(pboPath);
  hideLoading();

  if (result.success) {
    showAnalysisResults(result.data);
    showToast('Analise completa!', 'success');
  } else {
    showToast('Erro na analise: ' + result.error, 'error');
  }
}

async function selectAndAnalyzeFolder() {
  const folderPath = await window.pixpbo.selectFolder();
  if (!folderPath) return;

  currentPath = folderPath;
  showPath('analyze-path', folderPath);
  showLoading('Analisando pasta...');

  const result = await window.pixpbo.analyzeFolder(folderPath);
  hideLoading();

  if (result.success) {
    showAnalysisResults(result.data);
    showToast('Analise da pasta completa!', 'success');
  } else {
    showToast('Erro na analise: ' + result.error, 'error');
  }
}

function showAnalysisResults(data) {
  const resultsEl = document.getElementById('analyze-results');
  const titleEl = document.getElementById('analyze-title');
  const statsEl = document.getElementById('analyze-stats');
  const warningsEl = document.getElementById('analyze-warnings');
  const treeEl = document.getElementById('analyze-tree');

  resultsEl.classList.remove('hidden');
  titleEl.textContent = data.fileName || data.folderName;

  // Stats
  statsEl.innerHTML = `
    <div class="stat-item">
      <span class="stat-value">${data.totalFiles}</span>
      <span class="stat-label">Arquivos</span>
    </div>
    <div class="stat-item">
      <span class="stat-value">${data.fileSizeFormatted || data.totalSizeFormatted}</span>
      <span class="stat-label">Tamanho</span>
    </div>
    <div class="stat-item">
      <span class="stat-value">${data.scripts?.length || 0}</span>
      <span class="stat-label">Scripts .c</span>
    </div>
    <div class="stat-item">
      <span class="stat-value">${data.configs?.length || 0}</span>
      <span class="stat-label">Configs</span>
    </div>
  `;

  // Warnings
  if (data.warnings && data.warnings.length > 0) {
    warningsEl.classList.remove('hidden');
    warningsEl.innerHTML = data.warnings
      .map(w => `<div class="warning-item">&#9888; ${w}</div>`)
      .join('');
  } else {
    warningsEl.classList.add('hidden');
  }

  // File tree
  const entries = data.entries || [];
  treeEl.innerHTML = entries
    .slice(0, 200)
    .map(entry => {
      const name = entry.filename;
      const icon = getFileIcon(name);
      const size = entry.sizeFormatted || formatSize(entry.size || entry.dataSize || 0);
      return `<div class="tree-item">
        <span class="tree-name"><span class="tree-icon ${icon.type}">${icon.icon}</span>${name}</span>
        <span class="tree-size">${size}</span>
      </div>`;
    })
    .join('');

  if (entries.length > 200) {
    treeEl.innerHTML += `<div class="tree-item"><span class="tree-name">... e mais ${entries.length - 200} arquivos</span></div>`;
  }
}

// ---- Obfuscate ----
let obfuscatePath = null;

async function selectObfuscateFolder() {
  const folderPath = await window.pixpbo.selectFolder();
  if (!folderPath) return;

  obfuscatePath = folderPath;
  showPath('obfuscate-path', folderPath);
  document.getElementById('obfuscate-btn').disabled = false;
}

async function runObfuscation() {
  if (!obfuscatePath) return;

  const level = document.querySelector('input[name="obf-level"]:checked')?.value || 'medium';

  showLoading('Obfuscando scripts...');
  const result = await window.pixpbo.obfuscate({
    sourcePath: obfuscatePath,
    level: level,
  });
  hideLoading();

  if (result.success) {
    const data = result.data;
    document.getElementById('obfuscate-results').classList.remove('hidden');
    document.getElementById('obfuscate-results').innerHTML = `
      <div class="results-header">
        <h3>Obfuscacao Completa</h3>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivos processados</span>
        <span class="result-value">${data.filesProcessed}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivos ignorados</span>
        <span class="result-value">${data.filesSkipped}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Comentarios removidos</span>
        <span class="result-value">${data.commentsRemoved}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Variaveis renomeadas</span>
        <span class="result-value">${data.variablesRenamed}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Funcoes renomeadas</span>
        <span class="result-value">${data.functionsRenamed}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Compressao</span>
        <span class="result-value success">${data.compressionRatio}</span>
      </div>
    `;
    showToast('Obfuscacao completa!', 'success');
    lastReport = { steps: [{ name: 'Obfuscacao', status: 'success', data }] };
  } else {
    showToast('Erro na obfuscacao: ' + result.error, 'error');
  }
}

// ---- Clean ----
let cleanPath = null;

async function selectCleanFolder() {
  const folderPath = await window.pixpbo.selectFolder();
  if (!folderPath) return;

  cleanPath = folderPath;
  showPath('clean-path', folderPath);
  document.getElementById('clean-btn').disabled = false;
}

async function runCleaning() {
  if (!cleanPath) return;

  showLoading('Limpando arquivos...');
  const result = await window.pixpbo.cleanFiles(cleanPath);
  hideLoading();

  if (result.success) {
    const data = result.data;
    document.getElementById('clean-results').classList.remove('hidden');
    document.getElementById('clean-results').innerHTML = `
      <div class="results-header">
        <h3>Limpeza Completa</h3>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivos removidos</span>
        <span class="result-value">${data.filesRemovedCount}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Pastas removidas</span>
        <span class="result-value">${data.dirsRemovedCount}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Espaco liberado</span>
        <span class="result-value success">${data.totalFreedFormatted}</span>
      </div>
      ${data.filesRemoved.length > 0 ? `
      <h4 style="margin-top: 12px; color: var(--text-secondary); font-size: 13px;">Arquivos removidos:</h4>
      <div class="file-tree">
        ${data.filesRemoved.map(f => `<div class="tree-item"><span class="tree-name" style="color: var(--neon-red);">${f}</span></div>`).join('')}
      </div>
      ` : ''}
    `;
    showToast(`${data.filesRemovedCount} arquivos removidos!`, 'success');
  } else {
    showToast('Erro na limpeza: ' + result.error, 'error');
  }
}

// ---- Repack ----
let repackPath = null;

async function selectRepackFolder() {
  const folderPath = await window.pixpbo.selectFolder();
  if (!folderPath) return;

  repackPath = folderPath;
  showPath('repack-path', folderPath);
  document.getElementById('repack-btn').disabled = false;
}

async function runRepack() {
  if (!repackPath) return;

  showLoading('Reempacotando PBO...');
  const result = await window.pixpbo.repackPBO({
    sourcePath: repackPath,
  });
  hideLoading();

  if (result.success) {
    const data = result.data;
    document.getElementById('repack-results').classList.remove('hidden');
    document.getElementById('repack-results').innerHTML = `
      <div class="results-header">
        <h3>Reempacotamento Completo</h3>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivo de saida</span>
        <span class="result-value">${data.outputPath}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Tamanho original</span>
        <span class="result-value">${data.originalSizeFormatted}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Tamanho final</span>
        <span class="result-value success">${data.packedSizeFormatted}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivos incluidos</span>
        <span class="result-value">${data.filesIncluded}</span>
      </div>
    `;
    showToast('PBO reempacotado com sucesso!', 'success');
  } else {
    showToast('Erro no reempacotamento: ' + result.error, 'error');
  }
}

// ---- Sign ----
let signPath = null;

async function selectSignPBO() {
  const pboPath = await window.pixpbo.selectPBO();
  if (!pboPath) return;

  signPath = pboPath;
  showPath('sign-path', pboPath);
  document.getElementById('sign-btn').disabled = false;
}

async function runSigning() {
  if (!signPath) return;

  const keyName = document.getElementById('key-name-input').value.trim() || 'pixpbo';

  showLoading('Assinando mod...');
  const result = await window.pixpbo.signMod({
    pboPath: signPath,
    keyName: keyName,
  });
  hideLoading();

  if (result.success) {
    const data = result.data;
    document.getElementById('sign-results').classList.remove('hidden');
    document.getElementById('sign-results').innerHTML = `
      <div class="results-header">
        <h3>Assinatura Completa</h3>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivo .bisign</span>
        <span class="result-value">${data.bisignPath}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Arquivo .bikey</span>
        <span class="result-value">${data.bikeyPath}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Nome da chave</span>
        <span class="result-value">${data.keyName}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Hash do PBO</span>
        <span class="result-value">${data.pboHash}</span>
      </div>
      <div class="result-detail">
        <span class="result-label">Assinatura valida</span>
        <span class="result-value success">Sim</span>
      </div>
    `;
    showToast('Mod assinado com sucesso!', 'success');
  } else {
    showToast('Erro na assinatura: ' + result.error, 'error');
  }
}

// ---- Pipeline ----
let pipelinePath = null;

async function selectPipelineFolder() {
  const folderPath = await window.pixpbo.selectFolder();
  if (!folderPath) return;

  pipelinePath = folderPath;
  showPath('pipeline-path', folderPath);
  document.getElementById('pipeline-btn').disabled = false;
}

async function runPipeline() {
  if (!pipelinePath) return;

  const options = {
    clean: document.getElementById('pipe-clean').checked,
    obfuscate: document.getElementById('pipe-obfuscate').checked,
    repack: document.getElementById('pipe-repack').checked,
    sign: document.getElementById('pipe-sign').checked,
    obfuscationLevel: document.getElementById('pipe-obf-level').value,
    keyName: 'pixpbo',
  };

  // Show progress
  const progressEl = document.getElementById('pipeline-progress');
  progressEl.classList.remove('hidden');

  // Reset all steps
  document.querySelectorAll('.step-item').forEach(el => {
    el.className = 'step-item';
    el.querySelector('.step-status').textContent = 'Aguardando';
  });

  // Listen for progress events
  window.pixpbo.onPipelineProgress((data) => {
    const stepEl = document.getElementById(`step-${data.step}`);
    if (stepEl) {
      stepEl.className = `step-item ${data.status === 'running' ? 'running' : 'done'}`;
      stepEl.querySelector('.step-status').textContent =
        data.status === 'running' ? 'Executando...' : 'Concluido';
    }
  });

  document.getElementById('pipeline-btn').disabled = true;

  const result = await window.pixpbo.runPipeline({
    sourcePath: pipelinePath,
    options: options,
  });

  document.getElementById('pipeline-btn').disabled = false;

  if (result.success) {
    const data = result.data;
    const resultsEl = document.getElementById('pipeline-results');
    resultsEl.classList.remove('hidden');

    let html = '<div class="results-header"><h3>Pipeline Completo</h3></div>';

    for (const step of data.steps) {
      html += `<div class="result-detail">
        <span class="result-label">${step.name}</span>
        <span class="result-value ${step.status === 'success' ? 'success' : 'error'}">${step.status === 'success' ? 'OK' : 'Erro'}</span>
      </div>`;
    }

    if (data.startTime && data.endTime) {
      const duration = ((data.endTime - data.startTime) / 1000).toFixed(1);
      html += `<div class="result-detail">
        <span class="result-label">Tempo total</span>
        <span class="result-value">${duration}s</span>
      </div>`;
    }

    resultsEl.innerHTML = html;
    showToast('Pipeline completo com sucesso!', 'success');
    lastReport = data;

    // Update report tab
    updateReportTab(data);
  } else {
    showToast('Erro no pipeline: ' + result.error, 'error');
  }
}

function updateReportTab(data) {
  const reportEl = document.getElementById('report-content');
  let html = `
    <div class="results-header">
      <h3>Ultimo Relatorio</h3>
    </div>
  `;

  if (data.steps) {
    for (const step of data.steps) {
      html += `<div class="result-detail">
        <span class="result-label">${step.name}</span>
        <span class="result-value ${step.status === 'success' ? 'success' : 'error'}">${step.status === 'success' ? 'Concluido' : 'Erro'}</span>
      </div>`;

      if (step.data) {
        const details = [];
        if (step.data.filesProcessed !== undefined) details.push(`Arquivos: ${step.data.filesProcessed}`);
        if (step.data.filesRemovedCount !== undefined) details.push(`Removidos: ${step.data.filesRemovedCount}`);
        if (step.data.compressionRatio) details.push(`Compressao: ${step.data.compressionRatio}`);
        if (step.data.packedSizeFormatted) details.push(`Tamanho: ${step.data.packedSizeFormatted}`);

        if (details.length > 0) {
          html += `<div style="padding: 4px 16px; font-size: 12px; color: var(--text-muted);">${details.join(' | ')}</div>`;
        }
      }
    }
  }

  if (data.errors && data.errors.length > 0) {
    html += `<h4 style="margin-top: 16px; color: var(--neon-red);">Erros:</h4>`;
    for (const error of data.errors) {
      html += `<div class="result-detail"><span class="result-value error">${error}</span></div>`;
    }
  }

  reportEl.innerHTML = html;
}

// ---- Utility Functions ----
function showPath(elementId, path) {
  const el = document.getElementById(elementId);
  el.textContent = path;
  el.classList.remove('hidden');
}

function showLoading(text) {
  document.getElementById('loading-text').textContent = text || 'Processando...';
  document.getElementById('loading-overlay').classList.remove('hidden');
}

function hideLoading() {
  document.getElementById('loading-overlay').classList.add('hidden');
}

function showToast(message, type = 'info') {
  const container = document.getElementById('toast-container');
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;

  const icons = {
    success: '&#10003;',
    error: '&#10007;',
    info: '&#8505;',
    warning: '&#9888;',
  };

  toast.innerHTML = `<span>${icons[type] || icons.info}</span> ${message}`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.animation = 'toastOut 0.3s ease forwards';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

function getFileIcon(filename) {
  const ext = filename.split('.').pop()?.toLowerCase();
  const map = {
    c: { icon: '&#128220;', type: 'script' },
    cpp: { icon: '&#128220;', type: 'config' },
    hpp: { icon: '&#128220;', type: 'config' },
    bin: { icon: '&#9881;', type: 'config' },
    paa: { icon: '&#127912;', type: 'texture' },
    pac: { icon: '&#127912;', type: 'texture' },
    p3d: { icon: '&#128171;', type: 'model' },
    ogg: { icon: '&#127925;', type: 'sound' },
    wss: { icon: '&#127925;', type: 'sound' },
    pbo: { icon: '&#128230;', type: 'other' },
  };
  return map[ext] || { icon: '&#128196;', type: 'other' };
}

function formatSize(bytes) {
  if (bytes === 0) return '0 B';
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  return `${(bytes / Math.pow(1024, i)).toFixed(2)} ${sizes[i]}`;
}

// ---- Auto-format license input ----
document.addEventListener('DOMContentLoaded', () => {
  const licenseInput = document.getElementById('license-input');
  if (licenseInput) {
    licenseInput.addEventListener('input', (e) => {
      let val = e.target.value.toUpperCase().replace(/[^A-Z0-9-]/g, '');
      e.target.value = val;
    });

    licenseInput.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        validateLicense();
      }
    });
  }

  // Check saved license on startup
  checkSavedLicense();
});
