const fs = require('fs');
const path = require('path');
const { getLogger } = require('./logger');

/**
 * Generates detailed reports of processing operations
 */
class ReportGenerator {
  constructor(logsDir) {
    this.logsDir = logsDir;
    this.logger = getLogger();
  }

  /**
   * Generate a comprehensive report
   */
  async generate(data) {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const reportName = `pixpbo-report-${timestamp}`;

    // Generate text report
    const textReport = this._generateTextReport(data);
    const textPath = path.join(this.logsDir, `${reportName}.txt`);
    fs.writeFileSync(textPath, textReport, 'utf8');

    // Generate JSON report
    const jsonPath = path.join(this.logsDir, `${reportName}.json`);
    fs.writeFileSync(jsonPath, JSON.stringify(data, null, 2), 'utf8');

    // Generate HTML report
    const htmlReport = this._generateHTMLReport(data);
    const htmlPath = path.join(this.logsDir, `${reportName}.html`);
    fs.writeFileSync(htmlPath, htmlReport, 'utf8');

    this.logger.info(`Report generated: ${textPath}`);

    return {
      textPath,
      jsonPath,
      htmlPath,
      summary: this._generateSummary(data),
    };
  }

  _generateTextReport(data) {
    const lines = [
      '╔══════════════════════════════════════════════════════════════╗',
      '║                    PixPBO Protect - Relatório               ║',
      '╚══════════════════════════════════════════════════════════════╝',
      '',
      `Data: ${new Date().toLocaleString('pt-BR')}`,
      '',
    ];

    if (data.steps) {
      lines.push('═══ ETAPAS EXECUTADAS ═══');
      for (const step of data.steps) {
        const icon = step.status === 'success' ? '[OK]' : '[ERRO]';
        lines.push(`  ${icon} ${step.name}`);

        if (step.data) {
          if (step.data.filesProcessed !== undefined) {
            lines.push(`      Arquivos processados: ${step.data.filesProcessed}`);
          }
          if (step.data.filesRemovedCount !== undefined) {
            lines.push(`      Arquivos removidos: ${step.data.filesRemovedCount}`);
            lines.push(`      Espaço liberado: ${step.data.totalFreedFormatted}`);
          }
          if (step.data.variablesRenamed !== undefined) {
            lines.push(`      Variáveis renomeadas: ${step.data.variablesRenamed}`);
            lines.push(`      Funções renomeadas: ${step.data.functionsRenamed}`);
            lines.push(`      Comentários removidos: ${step.data.commentsRemoved}`);
            lines.push(`      Compressão: ${step.data.compressionRatio}`);
          }
          if (step.data.outputPath !== undefined) {
            lines.push(`      Saída: ${step.data.outputPath}`);
            lines.push(`      Tamanho original: ${step.data.originalSizeFormatted}`);
            lines.push(`      Tamanho final: ${step.data.packedSizeFormatted}`);
          }
          if (step.data.bisignPath !== undefined) {
            lines.push(`      Assinatura: ${step.data.bisignPath}`);
            lines.push(`      Chave: ${step.data.bikeyPath}`);
          }
        }
      }
      lines.push('');
    }

    if (data.errors && data.errors.length > 0) {
      lines.push('═══ ERROS ═══');
      for (const error of data.errors) {
        lines.push(`  [!] ${error}`);
      }
      lines.push('');
    }

    if (data.startTime && data.endTime) {
      const duration = ((data.endTime - data.startTime) / 1000).toFixed(1);
      lines.push(`Tempo total: ${duration}s`);
    }

    lines.push('');
    lines.push('═══════════════════════════════════════════════════════════════');
    lines.push('Gerado por PixPBO Protect v1.0.0');

    return lines.join('\n');
  }

  _generateHTMLReport(data) {
    return `<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>PixPBO Protect - Relatório</title>
  <style>
    body { background: #0a0a1a; color: #e0e0ff; font-family: 'Segoe UI', sans-serif; padding: 40px; }
    .container { max-width: 800px; margin: 0 auto; }
    h1 { color: #00d4ff; text-shadow: 0 0 20px rgba(0, 212, 255, 0.5); }
    h2 { color: #a855f7; border-bottom: 1px solid #a855f7; padding-bottom: 8px; }
    .step { background: rgba(255,255,255,0.05); border-radius: 8px; padding: 16px; margin: 8px 0; border-left: 3px solid #00d4ff; }
    .step.error { border-left-color: #ef4444; }
    .step-name { font-weight: bold; color: #00d4ff; }
    .detail { color: #a0a0c0; font-size: 14px; margin: 4px 0 4px 16px; }
    .error-box { background: rgba(239,68,68,0.1); border: 1px solid #ef4444; border-radius: 8px; padding: 12px; margin: 8px 0; }
    .footer { margin-top: 40px; color: #606080; text-align: center; font-size: 12px; }
    .badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 12px; }
    .badge.ok { background: rgba(34,197,94,0.2); color: #22c55e; }
    .badge.fail { background: rgba(239,68,68,0.2); color: #ef4444; }
  </style>
</head>
<body>
  <div class="container">
    <h1>PixPBO Protect</h1>
    <p>Relatório de Processamento</p>
    <p><strong>Data:</strong> ${new Date().toLocaleString('pt-BR')}</p>

    ${data.steps ? `
    <h2>Etapas Executadas</h2>
    ${data.steps.map(step => `
      <div class="step ${step.status !== 'success' ? 'error' : ''}">
        <span class="step-name">${step.name}</span>
        <span class="badge ${step.status === 'success' ? 'ok' : 'fail'}">${step.status === 'success' ? 'OK' : 'ERRO'}</span>
        ${step.data ? this._renderStepDetails(step.data) : ''}
      </div>
    `).join('')}
    ` : ''}

    ${data.errors && data.errors.length > 0 ? `
    <h2>Erros</h2>
    ${data.errors.map(e => `<div class="error-box">${e}</div>`).join('')}
    ` : ''}

    ${data.startTime && data.endTime ? `
    <p><strong>Tempo total:</strong> ${((data.endTime - data.startTime) / 1000).toFixed(1)}s</p>
    ` : ''}

    <div class="footer">Gerado por PixPBO Protect v1.0.0</div>
  </div>
</body>
</html>`;
  }

  _renderStepDetails(data) {
    const details = [];
    if (data.filesProcessed !== undefined) details.push(`Arquivos processados: ${data.filesProcessed}`);
    if (data.filesRemovedCount !== undefined) details.push(`Arquivos removidos: ${data.filesRemovedCount}`);
    if (data.totalFreedFormatted) details.push(`Espaço liberado: ${data.totalFreedFormatted}`);
    if (data.variablesRenamed !== undefined) details.push(`Variáveis renomeadas: ${data.variablesRenamed}`);
    if (data.functionsRenamed !== undefined) details.push(`Funções renomeadas: ${data.functionsRenamed}`);
    if (data.commentsRemoved !== undefined) details.push(`Comentários removidos: ${data.commentsRemoved}`);
    if (data.compressionRatio) details.push(`Compressão: ${data.compressionRatio}`);
    if (data.outputPath) details.push(`Saída: ${data.outputPath}`);
    if (data.originalSizeFormatted) details.push(`Tamanho original: ${data.originalSizeFormatted}`);
    if (data.packedSizeFormatted) details.push(`Tamanho final: ${data.packedSizeFormatted}`);
    if (data.bisignPath) details.push(`Assinatura: ${data.bisignPath}`);
    if (data.bikeyPath) details.push(`Chave pública: ${data.bikeyPath}`);

    return details.map(d => `<div class="detail">${d}</div>`).join('');
  }

  _generateSummary(data) {
    const steps = data.steps || [];
    const successes = steps.filter(s => s.status === 'success').length;
    const failures = steps.filter(s => s.status !== 'success').length;
    const errors = (data.errors || []).length;

    return {
      totalSteps: steps.length,
      successes,
      failures,
      errors,
      duration: data.startTime && data.endTime
        ? ((data.endTime - data.startTime) / 1000).toFixed(1) + 's'
        : 'N/A',
    };
  }
}

module.exports = { ReportGenerator };
