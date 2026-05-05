const { ActivityType } = require('discord.js');
const config = require('../config.json');

module.exports = {
  name: 'ready',
  once: true,
  execute(client) {
    console.log(`[PIXSupport] Bot online como ${client.user.tag}`);
    client.user.setActivity(`${config.serverName}`, { type: ActivityType.Watching });
  },
};
