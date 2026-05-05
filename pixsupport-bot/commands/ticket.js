const { SlashCommandBuilder, PermissionFlagsBits } = require('discord.js');
const { ticketPanelEmbed, ticketPanelButtons } = require('../utils/embeds');

module.exports = {
  data: new SlashCommandBuilder()
    .setName('ticket')
    .setDescription('Envia o painel de tickets no canal atual')
    .setDefaultMemberPermissions(PermissionFlagsBits.ManageGuild),

  async execute(interaction) {
    await interaction.reply({ content: 'Painel de tickets enviado!', ephemeral: true });
    await interaction.channel.send({
      embeds: [ticketPanelEmbed()],
      components: [ticketPanelButtons()],
    });
  },
};
