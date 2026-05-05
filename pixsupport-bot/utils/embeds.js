const { EmbedBuilder, ActionRowBuilder, ButtonBuilder, ButtonStyle } = require('discord.js');
const config = require('../config.json');

const color = parseInt(config.embedColor.replace('#', ''), 16);

function ticketPanelEmbed() {
  return new EmbedBuilder()
    .setColor(color)
    .setTitle(`${config.serverName} - Central de Atendimento`)
    .setDescription(
      'Bem-vindo ao sistema de suporte!\n\n' +
      'Selecione uma opcao abaixo para abrir um ticket:\n\n' +
      '> :gift: **Doacao** - Contribua com o servidor\n' +
      '> :wrench: **Suporte** - Problemas tecnicos ou duvidas gerais\n' +
      '> :bulb: **Duvida** - Perguntas rapidas\n\n' +
      '_Um membro da equipe ira atende-lo em breve._'
    )
    .setThumbnail(config.serverLogo)
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();
}

function ticketPanelButtons() {
  return new ActionRowBuilder().addComponents(
    new ButtonBuilder()
      .setCustomId('ticket_doacao')
      .setLabel('Doacao')
      .setEmoji('🎁')
      .setStyle(ButtonStyle.Success),
    new ButtonBuilder()
      .setCustomId('ticket_suporte')
      .setLabel('Suporte')
      .setEmoji('🛠️')
      .setStyle(ButtonStyle.Primary),
    new ButtonBuilder()
      .setCustomId('ticket_duvida')
      .setLabel('Duvida')
      .setEmoji('💡')
      .setStyle(ButtonStyle.Secondary),
  );
}

function ticketOpenEmbed(user, type, name, reason, description) {
  const typeLabels = {
    doacao: '🎁 Doacao',
    suporte: '🛠️ Suporte',
    duvida: '💡 Duvida',
  };

  return new EmbedBuilder()
    .setColor(color)
    .setTitle(`${config.serverName} - Ticket Aberto`)
    .setDescription(
      `Ticket criado com sucesso!\n\n` +
      `**Tipo:** ${typeLabels[type] || type}\n` +
      `**Nome:** ${name}\n` +
      `**Motivo:** ${reason}\n` +
      `**Descricao:** ${description}\n\n` +
      `_Aguarde, um membro da equipe ira atende-lo._`
    )
    .setThumbnail(config.serverLogo)
    .setFooter({ text: `Aberto por ${user.tag}`, iconURL: user.displayAvatarURL() })
    .setTimestamp();
}

function ticketActionButtons() {
  return new ActionRowBuilder().addComponents(
    new ButtonBuilder()
      .setCustomId('ticket_fechar')
      .setLabel('Fechar Ticket')
      .setEmoji('🔒')
      .setStyle(ButtonStyle.Danger),
    new ButtonBuilder()
      .setCustomId('ticket_assumir')
      .setLabel('Assumir Ticket')
      .setEmoji('📌')
      .setStyle(ButtonStyle.Primary),
  );
}

function donationEmbed() {
  return new EmbedBuilder()
    .setColor(color)
    .setTitle(`${config.serverName} - Doacao`)
    .setDescription(
      `Obrigado por querer contribuir com o servidor!\n\n` +
      `**Chave PIX / Link:**\n` +
      `\`\`\`${config.donationLink}\`\`\`\n\n` +
      `**Instrucoes:**\n${config.donationInstructions}\n\n` +
      `_Apos o pagamento, envie o comprovante aqui neste ticket._`
    )
    .setThumbnail(config.serverLogo)
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();
}

function ticketClosedEmbed(user, closedBy, transcript) {
  return new EmbedBuilder()
    .setColor(0xFF0000)
    .setTitle(`${config.serverName} - Ticket Encerrado`)
    .setDescription(
      `**Ticket de:** ${user.tag}\n` +
      `**Encerrado por:** ${closedBy.tag}\n` +
      `**Mensagens:** ${transcript.length}\n\n` +
      `_Log salvo com sucesso._`
    )
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();
}

function ticketAssumedEmbed(staff) {
  return new EmbedBuilder()
    .setColor(color)
    .setTitle('📌 Ticket Assumido')
    .setDescription(`Este ticket foi assumido por **${staff.tag}**.\n\n_O atendimento esta em andamento._`)
    .setTimestamp();
}

function dmOpenEmbed(channelName) {
  return new EmbedBuilder()
    .setColor(color)
    .setTitle(`${config.serverName} - Ticket Aberto`)
    .setDescription(
      `Seu ticket foi aberto com sucesso!\n\n` +
      `Canal: **#${channelName}**\n\n` +
      `_Aguarde o atendimento da equipe._`
    )
    .setFooter({ text: `${config.serverName} | PIXSupport` })
    .setTimestamp();
}

function dmCloseEmbed() {
  return new EmbedBuilder()
    .setColor(0xFF0000)
    .setTitle(`${config.serverName} - Ticket Encerrado`)
    .setDescription('Seu ticket foi encerrado.\n\n_Obrigado pelo contato!_')
    .setFooter({ text: `${config.serverName} | PIXSupport` })
    .setTimestamp();
}

function serverStatusEmbed(info) {
  const statusIcon = info.online ? '🟢' : '🔴';
  const statusText = info.online ? 'Online' : 'Offline';

  const embed = new EmbedBuilder()
    .setColor(info.online ? 0x00FF88 : 0xFF0000)
    .setTitle(`${config.serverName} - Status do Servidor`)
    .setThumbnail(config.serverLogo)
    .addFields(
      { name: 'Status', value: `${statusIcon} ${statusText}`, inline: true },
      { name: 'Jogadores', value: `${info.players}/${info.maxPlayers}`, inline: true },
      { name: 'Mapa', value: info.map || 'N/A', inline: true },
      { name: 'IP', value: `\`${config.dayz.serverIp}:${config.dayz.serverPort}\``, inline: true },
    )
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();

  if (info.online) {
    embed.addFields({
      name: 'Conectar',
      value: `[Clique para conectar](${config.dayz.connectUrl.replace('{ip}', config.dayz.serverIp).replace('{port}', config.dayz.serverPort)})`,
      inline: false,
    });
  }

  return embed;
}

function connectButton() {
  return new ActionRowBuilder().addComponents(
    new ButtonBuilder()
      .setLabel('Conectar ao Servidor')
      .setEmoji('🎮')
      .setStyle(ButtonStyle.Link)
      .setURL(config.dayz.connectUrl
        .replace('{ip}', config.dayz.serverIp)
        .replace('{port}', config.dayz.serverPort)),
  );
}

function topKillsEmbed(players) {
  const list = players.length > 0
    ? players.map((p, i) => `**${i + 1}.** ${p.name} - ${p.kills} kills`).join('\n')
    : '_Sem dados disponveis._';

  return new EmbedBuilder()
    .setColor(color)
    .setTitle(`${config.serverName} - Top Kills`)
    .setDescription(list)
    .setThumbnail(config.serverLogo)
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();
}

function vipEmbed(user, days) {
  return new EmbedBuilder()
    .setColor(0xFFD700)
    .setTitle(`${config.serverName} - VIP Ativado`)
    .setDescription(
      `VIP ativado para **${user.tag}**!\n\n` +
      `**Duracao:** ${days} dias\n\n` +
      `_O cargo VIP foi atribuido automaticamente._`
    )
    .setThumbnail(config.serverLogo)
    .setFooter({ text: `${config.serverName} | PIXSupport`, iconURL: config.serverLogo })
    .setTimestamp();
}

module.exports = {
  ticketPanelEmbed,
  ticketPanelButtons,
  ticketOpenEmbed,
  ticketActionButtons,
  donationEmbed,
  ticketClosedEmbed,
  ticketAssumedEmbed,
  dmOpenEmbed,
  dmCloseEmbed,
  serverStatusEmbed,
  connectButton,
  topKillsEmbed,
  vipEmbed,
};
