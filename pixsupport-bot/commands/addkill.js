const { SlashCommandBuilder, PermissionFlagsBits } = require('discord.js');
const { addKill } = require('../utils/dayz');

module.exports = {
  data: new SlashCommandBuilder()
    .setName('addkill')
    .setDescription('Adiciona kill ao ranking de um jogador (staff only)')
    .setDefaultMemberPermissions(PermissionFlagsBits.ManageGuild)
    .addStringOption(opt =>
      opt.setName('jogador')
        .setDescription('Nome do jogador')
        .setRequired(true)
    )
    .addIntegerOption(opt =>
      opt.setName('quantidade')
        .setDescription('Quantidade de kills (padrao: 1)')
        .setRequired(false)
        .setMinValue(1)
    ),

  async execute(interaction) {
    const player = interaction.options.getString('jogador');
    const amount = interaction.options.getInteger('quantidade') || 1;

    for (let i = 0; i < amount; i++) {
      addKill(player);
    }

    await interaction.reply({
      content: `+${amount} kill(s) registrada(s) para **${player}**.`,
      ephemeral: true,
    });
  },
};
