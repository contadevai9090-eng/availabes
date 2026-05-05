const { SlashCommandBuilder } = require('discord.js');
const { getTopKills } = require('../utils/dayz');
const { topKillsEmbed } = require('../utils/embeds');

module.exports = {
  data: new SlashCommandBuilder()
    .setName('topkills')
    .setDescription('Mostra o ranking de kills do servidor'),

  async execute(interaction) {
    const players = getTopKills(10);
    await interaction.reply({ embeds: [topKillsEmbed(players)] });
  },
};
