const {
  ModalBuilder,
  TextInputBuilder,
  TextInputStyle,
  ActionRowBuilder,
  ChannelType,
  PermissionFlagsBits,
} = require('discord.js');
const config = require('../config.json');
const { isOnCooldown, setCooldown } = require('../utils/cooldown');
const { generateTranscript } = require('../utils/transcript');
const {
  ticketOpenEmbed,
  ticketActionButtons,
  donationEmbed,
  ticketClosedEmbed,
  ticketAssumedEmbed,
  dmOpenEmbed,
  dmCloseEmbed,
} = require('../utils/embeds');

module.exports = {
  name: 'interactionCreate',
  once: false,

  async execute(interaction, client) {
    // Slash commands
    if (interaction.isChatInputCommand()) {
      const command = client.commands.get(interaction.commandName);
      if (!command) return;

      try {
        await command.execute(interaction);
      } catch (error) {
        console.error(`[PIXSupport] Erro no comando ${interaction.commandName}:`, error);
        const reply = { content: 'Ocorreu um erro ao executar este comando.', ephemeral: true };
        if (interaction.replied || interaction.deferred) {
          await interaction.followUp(reply);
        } else {
          await interaction.reply(reply);
        }
      }
      return;
    }

    // Button interactions (ticket panel)
    if (interaction.isButton()) {
      const ticketTypes = ['ticket_doacao', 'ticket_suporte', 'ticket_duvida'];

      if (ticketTypes.includes(interaction.customId)) {
        await handleTicketButton(interaction, client);
        return;
      }

      if (interaction.customId === 'ticket_fechar') {
        await handleCloseTicket(interaction, client);
        return;
      }

      if (interaction.customId === 'ticket_assumir') {
        await handleAssumeTicket(interaction, client);
        return;
      }
    }

    // Modal submissions
    if (interaction.isModalSubmit()) {
      if (interaction.customId.startsWith('modal_ticket_')) {
        await handleModalSubmit(interaction, client);
        return;
      }
    }
  },
};

async function handleTicketButton(interaction, client) {
  const userId = interaction.user.id;
  const type = interaction.customId.replace('ticket_', '');

  // Anti-spam: check cooldown
  const remaining = isOnCooldown(userId);
  if (remaining > 0) {
    return interaction.reply({
      content: `Aguarde ${remaining} segundos antes de abrir outro ticket.`,
      ephemeral: true,
    });
  }

  // Check if user already has an active ticket
  if (client.activeTickets.has(userId)) {
    return interaction.reply({
      content: 'Voce ja possui um ticket aberto. Feche-o antes de abrir outro.',
      ephemeral: true,
    });
  }

  // Open modal
  const modal = new ModalBuilder()
    .setCustomId(`modal_ticket_${type}`)
    .setTitle('Abrir Ticket');

  const nameInput = new TextInputBuilder()
    .setCustomId('ticket_name')
    .setLabel('Seu nome')
    .setStyle(TextInputStyle.Short)
    .setPlaceholder('Digite seu nome...')
    .setRequired(true)
    .setMaxLength(50);

  const reasonInput = new TextInputBuilder()
    .setCustomId('ticket_reason')
    .setLabel('Motivo')
    .setStyle(TextInputStyle.Short)
    .setPlaceholder('Resumo do motivo...')
    .setRequired(true)
    .setMaxLength(100);

  const descInput = new TextInputBuilder()
    .setCustomId('ticket_description')
    .setLabel('Descricao')
    .setStyle(TextInputStyle.Paragraph)
    .setPlaceholder('Descreva detalhadamente...')
    .setRequired(true)
    .setMaxLength(1000);

  modal.addComponents(
    new ActionRowBuilder().addComponents(nameInput),
    new ActionRowBuilder().addComponents(reasonInput),
    new ActionRowBuilder().addComponents(descInput),
  );

  await interaction.showModal(modal);
}

async function handleModalSubmit(interaction, client) {
  const type = interaction.customId.replace('modal_ticket_', '');
  const user = interaction.user;

  const name = interaction.fields.getTextInputValue('ticket_name');
  const reason = interaction.fields.getTextInputValue('ticket_reason');
  const description = interaction.fields.getTextInputValue('ticket_description');

  // Set cooldown
  setCooldown(user.id);

  // Create ticket channel
  const guild = interaction.guild;
  const channelName = `ticket-${user.username.toLowerCase().replace(/[^a-z0-9]/g, '')}`;

  try {
    const permissionOverwrites = [
      {
        id: guild.id,
        deny: [PermissionFlagsBits.ViewChannel],
      },
      {
        id: user.id,
        allow: [
          PermissionFlagsBits.ViewChannel,
          PermissionFlagsBits.SendMessages,
          PermissionFlagsBits.AttachFiles,
          PermissionFlagsBits.ReadMessageHistory,
        ],
      },
    ];

    // Add staff role permission if configured
    if (config.staffRoleId && config.staffRoleId !== 'ID_CARGO_STAFF') {
      permissionOverwrites.push({
        id: config.staffRoleId,
        allow: [
          PermissionFlagsBits.ViewChannel,
          PermissionFlagsBits.SendMessages,
          PermissionFlagsBits.AttachFiles,
          PermissionFlagsBits.ReadMessageHistory,
          PermissionFlagsBits.ManageMessages,
        ],
      });
    }

    const channelOptions = {
      name: channelName,
      type: ChannelType.GuildText,
      permissionOverwrites,
      topic: `Ticket de ${user.tag} | Tipo: ${type} | Motivo: ${reason}`,
    };

    // Set category if configured
    if (config.ticketCategoryId && config.ticketCategoryId !== 'ID_CATEGORIA_TICKET') {
      channelOptions.parent = config.ticketCategoryId;
    }

    const ticketChannel = await guild.channels.create(channelOptions);

    // Track active ticket
    client.activeTickets.set(user.id, {
      channelId: ticketChannel.id,
      userId: user.id,
      type,
      openedAt: new Date(),
    });

    // Send ticket embed
    const embeds = [ticketOpenEmbed(user, type, name, reason, description)];

    // If donation, also send donation info
    if (type === 'doacao') {
      embeds.push(donationEmbed());
    }

    await ticketChannel.send({
      content: `<@${user.id}>${config.staffRoleId && config.staffRoleId !== 'ID_CARGO_STAFF' ? ` | <@&${config.staffRoleId}>` : ''}`,
      embeds,
      components: [ticketActionButtons()],
    });

    // Reply to modal
    await interaction.reply({
      content: `Ticket criado com sucesso! Acesse: <#${ticketChannel.id}>`,
      ephemeral: true,
    });

    // DM user
    try {
      await user.send({ embeds: [dmOpenEmbed(channelName)] });
    } catch {
      // User may have DMs disabled
    }
  } catch (error) {
    console.error('[PIXSupport] Erro ao criar ticket:', error);
    await interaction.reply({
      content: 'Erro ao criar o ticket. Verifique as permissoes do bot.',
      ephemeral: true,
    });
  }
}

async function handleCloseTicket(interaction, client) {
  const channel = interaction.channel;
  const closedBy = interaction.user;

  // Check if this is a ticket channel
  if (!channel.name.startsWith('ticket-')) {
    return interaction.reply({
      content: 'Este comando so pode ser usado em canais de ticket.',
      ephemeral: true,
    });
  }

  // Check permission: user must be staff or ticket owner
  const isStaff = config.staffRoleId && config.staffRoleId !== 'ID_CARGO_STAFF'
    ? interaction.member.roles.cache.has(config.staffRoleId)
    : false;
  const isAdmin = interaction.member.permissions.has(PermissionFlagsBits.ManageGuild);

  // Find ticket owner from activeTickets
  let ticketOwner = null;
  for (const [userId, ticket] of client.activeTickets.entries()) {
    if (ticket.channelId === channel.id) {
      ticketOwner = userId;
      break;
    }
  }

  const isOwner = ticketOwner === closedBy.id;

  if (!isStaff && !isAdmin && !isOwner) {
    return interaction.reply({
      content: 'Voce nao tem permissao para fechar este ticket.',
      ephemeral: true,
    });
  }

  await interaction.reply({ content: 'Fechando ticket e gerando log...' });

  // Generate transcript
  const { content: transcriptText, messages } = await generateTranscript(channel);

  // Send log to log channel
  if (config.logChannelId && config.logChannelId !== 'ID_CANAL_LOGS') {
    const logChannel = interaction.guild.channels.cache.get(config.logChannelId);
    if (logChannel) {
      const ownerUser = ticketOwner
        ? await interaction.client.users.fetch(ticketOwner).catch(() => ({ tag: 'Desconhecido' }))
        : { tag: 'Desconhecido' };

      await logChannel.send({
        embeds: [ticketClosedEmbed(ownerUser, closedBy, messages)],
        files: [{
          attachment: Buffer.from(transcriptText, 'utf-8'),
          name: `transcript-${channel.name}-${Date.now()}.txt`,
        }],
      });
    }
  }

  // DM ticket owner
  if (ticketOwner) {
    try {
      const owner = await interaction.client.users.fetch(ticketOwner);
      await owner.send({ embeds: [dmCloseEmbed()] });
    } catch {
      // User may have DMs disabled
    }
    client.activeTickets.delete(ticketOwner);
  }

  // Delete channel after a delay
  setTimeout(async () => {
    try {
      await channel.delete('Ticket encerrado');
    } catch (error) {
      console.error('[PIXSupport] Erro ao deletar canal:', error);
    }
  }, 5000);
}

async function handleAssumeTicket(interaction, client) {
  const channel = interaction.channel;
  const staff = interaction.user;

  // Check if this is a ticket channel
  if (!channel.name.startsWith('ticket-')) {
    return interaction.reply({
      content: 'Este comando so pode ser usado em canais de ticket.',
      ephemeral: true,
    });
  }

  // Check staff permission
  const isStaff = config.staffRoleId && config.staffRoleId !== 'ID_CARGO_STAFF'
    ? interaction.member.roles.cache.has(config.staffRoleId)
    : false;
  const isAdmin = interaction.member.permissions.has(PermissionFlagsBits.ManageGuild);

  if (!isStaff && !isAdmin) {
    return interaction.reply({
      content: 'Apenas membros da equipe podem assumir tickets.',
      ephemeral: true,
    });
  }

  await interaction.reply({ embeds: [ticketAssumedEmbed(staff)] });
}
