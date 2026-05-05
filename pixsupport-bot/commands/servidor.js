const { SlashCommandBuilder } = require('discord.js');
const { queryServer } = require('../utils/dayz');
const { serverStatusEmbed, connectButton } = require('../utils/embeds');

module.exports = {
  data: new SlashCommandBuilder()
    .setName('servidor')
    .setDescription('Mostra o status do servidor DayZ'),

  async execute(interaction) {
    await interaction.deferReply();

    const info = await queryServer();
    const embeds = [serverStatusEmbed(info)];
    const components = info.online ? [connectButton()] : [];

    await interaction.editReply({ embeds, components });
  },
};
