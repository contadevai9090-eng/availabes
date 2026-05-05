const config = require('../config.json');

let GameDig;
try {
  GameDig = require('gamedig');
} catch {
  GameDig = null;
}

const topKillsStore = new Map();

async function queryServer() {
  if (!GameDig) {
    return {
      online: false,
      players: 0,
      maxPlayers: 0,
      map: 'N/A',
      playerList: [],
      error: 'gamedig not installed',
    };
  }

  try {
    const state = await GameDig.query({
      type: 'dayz',
      host: config.dayz.serverIp,
      port: config.dayz.queryPort || config.dayz.serverPort,
    });

    return {
      online: true,
      players: state.numplayers ?? state.players.length,
      maxPlayers: state.maxplayers,
      map: state.map || 'Chernarus',
      playerList: state.players.map(p => ({ name: p.name || 'Unknown' })),
    };
  } catch {
    return {
      online: false,
      players: 0,
      maxPlayers: 0,
      map: 'N/A',
      playerList: [],
    };
  }
}

function addKill(playerName) {
  const current = topKillsStore.get(playerName) || 0;
  topKillsStore.set(playerName, current + 1);
}

function getTopKills(limit = 10) {
  const sorted = [...topKillsStore.entries()]
    .sort((a, b) => b[1] - a[1])
    .slice(0, limit)
    .map(([name, kills]) => ({ name, kills }));
  return sorted;
}

module.exports = { queryServer, addKill, getTopKills };
