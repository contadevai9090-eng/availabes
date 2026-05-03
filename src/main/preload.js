const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('pixpbo', {
  // Window controls
  minimize: () => ipcRenderer.send('window-minimize'),
  maximize: () => ipcRenderer.send('window-maximize'),
  close: () => ipcRenderer.send('window-close'),

  // Dialogs
  selectFolder: () => ipcRenderer.invoke('select-folder'),
  selectPBO: () => ipcRenderer.invoke('select-pbo'),

  // Core functions
  analyzePBO: (pboPath) => ipcRenderer.invoke('analyze-pbo', pboPath),
  analyzeFolder: (folderPath) => ipcRenderer.invoke('analyze-folder', folderPath),
  obfuscate: (options) => ipcRenderer.invoke('obfuscate', options),
  cleanFiles: (targetPath) => ipcRenderer.invoke('clean-files', targetPath),
  repackPBO: (options) => ipcRenderer.invoke('repack-pbo', options),
  signMod: (options) => ipcRenderer.invoke('sign-mod', options),
  generateReport: (data) => ipcRenderer.invoke('generate-report', data),

  // Pipeline
  runPipeline: (options) => ipcRenderer.invoke('run-pipeline', options),
  onPipelineProgress: (callback) => {
    ipcRenderer.on('pipeline-progress', (event, data) => callback(data));
  },

  // License
  validateLicense: (key) => ipcRenderer.invoke('validate-license', key),
  saveLicense: (key) => ipcRenderer.invoke('save-license', key),
  getLicense: () => ipcRenderer.invoke('get-license'),

  // Utility
  getAppPaths: () => ipcRenderer.invoke('get-app-paths'),
  openPath: (path) => ipcRenderer.invoke('open-path', path),
});
