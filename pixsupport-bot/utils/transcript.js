async function generateTranscript(channel) {
  const messages = [];
  let lastId;

  while (true) {
    const options = { limit: 100 };
    if (lastId) options.before = lastId;

    const fetched = await channel.messages.fetch(options);
    if (fetched.size === 0) break;

    for (const msg of fetched.values()) {
      messages.push(msg);
    }
    lastId = fetched.last().id;
  }

  messages.reverse();

  const lines = [
    `=== Transcript: #${channel.name} ===`,
    `Data: ${new Date().toLocaleString('pt-BR')}`,
    `Total de mensagens: ${messages.length}`,
    '='.repeat(50),
    '',
  ];

  for (const msg of messages) {
    const time = msg.createdAt.toLocaleString('pt-BR');
    const author = msg.author ? msg.author.tag : 'Sistema';
    const content = msg.content || '';
    const embeds = msg.embeds.length > 0 ? ` [${msg.embeds.length} embed(s)]` : '';
    const attachments = msg.attachments.size > 0
      ? ` [Anexos: ${msg.attachments.map(a => a.url).join(', ')}]`
      : '';

    lines.push(`[${time}] ${author}: ${content}${embeds}${attachments}`);
  }

  return {
    content: lines.join('\n'),
    messages,
  };
}

module.exports = { generateTranscript };
