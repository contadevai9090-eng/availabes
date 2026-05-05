const { SlashCommandBuilder, PermissionFlagsBits } = require('discord.js');
const config = require('../config.json');
const { vipEmbed } = require('../utils/embeds');

module.exports = {
  data: new SlashCommandBuilder()
    .setName('vip')
    .setDescription('Ativa VIP para um usuario (staff only)')
    .setDefaultMemberPermissions(PermissionFlagsBits.ManageRoles)
    .addUserOption(opt =>
      opt.setName('usuario')
        .setDescription('Usuario para receber VIP')
        .setRequired(true)
    )
    .addIntegerOption(opt =>
      opt.setName('dias')
        .setDescription('Duracao em dias (padrao: 30)')
        .setRequired(false)
        .setMinValue(1)
        .setMaxValue(365)
    ),

  async execute(interaction) {
    const user = interaction.options.getUser('usuario');
    const days = interaction.options.getInteger('dias') || 30;
    const member = await interaction.guild.members.fetch(user.id).catch(() => null);

    if (!member) {
      return interaction.reply({ content: 'Usuario nao encontrado no servidor.', ephemeral: true });
    }

    const vipRoleId = config.dayz.vipRoleId;
    if (!vipRoleId || vipRoleId === 'ID_CARGO_VIP') {
      return interaction.reply({ content: 'Cargo VIP nao configurado no config.json.', ephemeral: true });
    }

    try {
      await member.roles.add(vipRoleId);
      await interaction.reply({ embeds: [vipEmbed(user, days)] });

      try {
        await user.send({
          content: `Parabens! Voce recebeu **VIP** por **${days} dias** no servidor **${config.serverName}**!`,
        });
      } catch {
        // User may have DMs disabled
      }
    } catch (error) {
      await interaction.reply({
        content: `Erro ao atribuir cargo VIP: ${error.message}`,
        ephemeral: true,
      });
    }
  },
};
