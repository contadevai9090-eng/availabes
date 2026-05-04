/**
 * PixPBO Protect - Discord Bot for License Key Management
 *
 * Commands:
 *   /gerar-key [dias] [tier]  - Generate a new license key
 *   /verificar-key [key]      - Verify a license key
 *   /revogar-key [key]        - Revoke a license key
 *   /listar-keys              - List all active keys
 *   /minha-key                - Show user's active key
 *
 * Setup:
 *   1. Create a Discord bot at https://discord.com/developers
 *   2. Copy the token to config.json
 *   3. Set the admin role ID in config.json
 *   4. Run: npm install && node bot.js
 */

const { Client, GatewayIntentBits, SlashCommandBuilder, REST, Routes, EmbedBuilder, PermissionFlagsBits } = require('discord.js');
const Database = require('better-sqlite3');
const crypto = require('crypto');
const path = require('path');
const fs = require('fs');

// ---- Configuration ----
const CONFIG_PATH = path.join(__dirname, 'config.json');
let config = {
  token: 'YOUR_BOT_TOKEN_HERE',
  clientId: 'YOUR_CLIENT_ID_HERE',
  guildId: 'YOUR_GUILD_ID_HERE',
  adminRoleId: 'YOUR_ADMIN_ROLE_ID_HERE',
  prefix: '!',
  maxKeysPerUser: 3,
};

if (fs.existsSync(CONFIG_PATH)) {
  config = { ...config, ...JSON.parse(fs.readFileSync(CONFIG_PATH, 'utf8')) };
}

// ---- Database Setup ----
const DB_PATH = path.join(__dirname, 'licenses.db');
const db = new Database(DB_PATH);

db.exec(`
  CREATE TABLE IF NOT EXISTS licenses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    key TEXT UNIQUE NOT NULL,
    discord_user_id TEXT NOT NULL,
    discord_username TEXT NOT NULL,
    tier TEXT DEFAULT 'pro',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    revoked INTEGER DEFAULT 0,
    revoked_at DATETIME,
    revoked_by TEXT,
    activated INTEGER DEFAULT 0,
    activated_at DATETIME,
    machine_id TEXT
  );

  CREATE INDEX IF NOT EXISTS idx_key ON licenses(key);
  CREATE INDEX IF NOT EXISTS idx_user ON licenses(discord_user_id);
`);

// ---- Key Generation ----
const SECRET = 'pixpbo-protect-2024-secure-key';

function generateKey(durationDays = 30, tier = 'pro') {
  const expiresAt = Date.now() + (durationDays * 24 * 60 * 60 * 1000);
  const features = tier === 'pro' ? 0xFF : 0x0F;

  const expHex = expiresAt.toString(16).toUpperCase().padStart(12, '0');
  const tierCode = tier === 'pro' ? 'P' : 'B';
  const featHex = features.toString(16).toUpperCase().padStart(2, '0');

  const seg1 = expHex.substring(0, 4);
  const seg2 = expHex.substring(4, 8);
  const seg3 = expHex.substring(8, 12);

  // First 2 chars of seg4 encode tier+feature prefix
  const seg4Prefix = `${tierCode}${featHex.charAt(0)}`;

  // Calculate checksum using the same format as client verification
  const rawSeg4 = seg4Prefix + 'XX';
  const rawKey = `PIXPBO-${seg1}-${seg2}-${seg3}-${rawSeg4}`;
  const checksum = crypto
    .createHmac('sha256', SECRET)
    .update(rawKey)
    .digest('hex')
    .substring(0, 2)
    .toUpperCase();

  const finalSeg4 = seg4Prefix + checksum;
  return `PIXPBO-${seg1}-${seg2}-${seg3}-${finalSeg4}`;
}

function validateKeyFormat(key) {
  return /^PIXPBO-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$/.test(key);
}

// ---- Discord Client ----
const client = new Client({
  intents: [
    GatewayIntentBits.Guilds,
    GatewayIntentBits.GuildMessages,
  ],
});

// ---- Slash Commands ----
const commands = [
  new SlashCommandBuilder()
    .setName('gerar-key')
    .setDescription('Gera uma nova chave de licenca PixPBO')
    .addIntegerOption(opt =>
      opt.setName('dias')
        .setDescription('Duracao em dias (padrao: 30)')
        .setRequired(false)
        .setMinValue(1)
        .setMaxValue(365)
    )
    .addStringOption(opt =>
      opt.setName('tier')
        .setDescription('Nivel da licenca')
        .setRequired(false)
        .addChoices(
          { name: 'Pro (Todas funcoes)', value: 'pro' },
          { name: 'Basic (Funcoes basicas)', value: 'basic' }
        )
    )
    .addUserOption(opt =>
      opt.setName('usuario')
        .setDescription('Usuario para quem gerar a chave (admin only)')
        .setRequired(false)
    ),

  new SlashCommandBuilder()
    .setName('verificar-key')
    .setDescription('Verifica o status de uma chave de licenca')
    .addStringOption(opt =>
      opt.setName('key')
        .setDescription('Chave para verificar')
        .setRequired(true)
    ),

  new SlashCommandBuilder()
    .setName('revogar-key')
    .setDescription('Revoga uma chave de licenca (admin only)')
    .addStringOption(opt =>
      opt.setName('key')
        .setDescription('Chave para revogar')
        .setRequired(true)
    ),

  new SlashCommandBuilder()
    .setName('listar-keys')
    .setDescription('Lista todas as chaves ativas (admin only)'),

  new SlashCommandBuilder()
    .setName('minha-key')
    .setDescription('Mostra suas chaves ativas'),

  new SlashCommandBuilder()
    .setName('pixpbo-help')
    .setDescription('Mostra os comandos disponiveis'),
];

// ---- Register Commands ----
async function registerCommands() {
  const rest = new REST({ version: '10' }).setToken(config.token);
  try {
    console.log('Registrando comandos slash...');
    await rest.put(
      Routes.applicationGuildCommands(config.clientId, config.guildId),
      { body: commands.map(cmd => cmd.toJSON()) }
    );
    console.log('Comandos registrados com sucesso!');
  } catch (error) {
    console.error('Erro ao registrar comandos:', error);
  }
}

// ---- Helper Functions ----
function isAdmin(member) {
  return member.roles.cache.has(config.adminRoleId) ||
         member.permissions.has(PermissionFlagsBits.Administrator);
}

function createEmbed(title, description, color = 0x00d4ff) {
  return new EmbedBuilder()
    .setTitle(title)
    .setDescription(description)
    .setColor(color)
    .setFooter({ text: 'PixPBO Protect | Sistema de Licencas' })
    .setTimestamp();
}

// ---- Event Handlers ----
client.once('ready', async () => {
  console.log(`\n${'='.repeat(50)}`);
  console.log(`  PixPBO Protect - Discord Bot`);
  console.log(`  Bot online como: ${client.user.tag}`);
  console.log(`  Servidores: ${client.guilds.cache.size}`);
  console.log(`${'='.repeat(50)}\n`);

  client.user.setActivity('PixPBO Protect | /pixpbo-help');
  await registerCommands();
});

client.on('interactionCreate', async (interaction) => {
  if (!interaction.isChatInputCommand()) return;

  const { commandName, member, user } = interaction;

  try {
    switch (commandName) {
      case 'gerar-key': {
        // Check permissions - admin can generate for others
        const targetUser = interaction.options.getUser('usuario');
        if (targetUser && !isAdmin(member)) {
          return interaction.reply({
            embeds: [createEmbed('Sem Permissao', 'Apenas administradores podem gerar chaves para outros usuarios.', 0xef4444)],
            ephemeral: true,
          });
        }

        const targetId = targetUser?.id || user.id;
        const targetName = targetUser?.username || user.username;

        // Check max keys
        const activeKeys = db.prepare(
          "SELECT COUNT(*) as count FROM licenses WHERE discord_user_id = ? AND revoked = 0 AND expires_at > datetime('now')"
        ).get(targetId);

        if (activeKeys.count >= config.maxKeysPerUser && !isAdmin(member)) {
          return interaction.reply({
            embeds: [createEmbed('Limite Atingido', `Voce ja possui ${activeKeys.count} chave(s) ativa(s). Maximo: ${config.maxKeysPerUser}`, 0xef4444)],
            ephemeral: true,
          });
        }

        const days = interaction.options.getInteger('dias') || 30;
        const tier = interaction.options.getString('tier') || 'pro';
        const key = generateKey(days, tier);
        const expiresAt = new Date(Date.now() + days * 24 * 60 * 60 * 1000).toISOString();

        // Save to database
        db.prepare(
          'INSERT INTO licenses (key, discord_user_id, discord_username, tier, expires_at) VALUES (?, ?, ?, ?, ?)'
        ).run(key, targetId, targetName, tier, expiresAt);

        const embed = createEmbed(
          'Chave Gerada com Sucesso!',
          `Sua nova chave de licenca PixPBO Protect:`,
          0x22c55e
        )
        .addFields(
          { name: 'Chave', value: `\`\`\`${key}\`\`\``, inline: false },
          { name: 'Tier', value: tier === 'pro' ? 'Pro' : 'Basic', inline: true },
          { name: 'Validade', value: `${days} dias`, inline: true },
          { name: 'Expira em', value: new Date(expiresAt).toLocaleDateString('pt-BR'), inline: true },
          { name: 'Usuario', value: targetName, inline: true },
          { name: 'Uso Unico', value: 'Sim - So pode ser ativada em 1 PC', inline: true },
        );

        // Send key privately
        await interaction.reply({
          embeds: [embed],
          ephemeral: true,
        });
        break;
      }

      case 'verificar-key': {
        const key = interaction.options.getString('key').trim().toUpperCase();

        if (!validateKeyFormat(key)) {
          return interaction.reply({
            embeds: [createEmbed('Formato Invalido', 'O formato da chave deve ser: `PIXPBO-XXXX-XXXX-XXXX-XXXX`', 0xef4444)],
            ephemeral: true,
          });
        }

        const license = db.prepare(
          'SELECT * FROM licenses WHERE key = ?'
        ).get(key);

        if (!license) {
          return interaction.reply({
            embeds: [createEmbed('Chave Nao Encontrada', 'Esta chave nao existe no sistema.', 0xef4444)],
            ephemeral: true,
          });
        }

        const isExpired = new Date(license.expires_at) < new Date();
        const isRevoked = license.revoked === 1;
        const isActivated = license.activated === 1;
        const status = isRevoked ? 'Revogada' : isExpired ? 'Expirada' : isActivated ? 'Ativa (Em uso)' : 'Ativa (Nao ativada)';
        const color = isRevoked || isExpired ? 0xef4444 : 0x22c55e;

        const embed = createEmbed('Status da Chave', '', color)
          .addFields(
            { name: 'Chave', value: `\`${key}\``, inline: false },
            { name: 'Status', value: status, inline: true },
            { name: 'Tier', value: license.tier === 'pro' ? 'Pro' : 'Basic', inline: true },
            { name: 'Uso Unico', value: isActivated ? 'Ja ativada' : 'Disponivel', inline: true },
            { name: 'Criada em', value: new Date(license.created_at).toLocaleDateString('pt-BR'), inline: true },
            { name: 'Expira em', value: new Date(license.expires_at).toLocaleDateString('pt-BR'), inline: true },
            { name: 'Usuario', value: license.discord_username, inline: true },
          );

        await interaction.reply({ embeds: [embed], ephemeral: true });
        break;
      }

      case 'revogar-key': {
        if (!isAdmin(member)) {
          return interaction.reply({
            embeds: [createEmbed('Sem Permissao', 'Apenas administradores podem revogar chaves.', 0xef4444)],
            ephemeral: true,
          });
        }

        const key = interaction.options.getString('key').trim().toUpperCase();
        const license = db.prepare('SELECT * FROM licenses WHERE key = ?').get(key);

        if (!license) {
          return interaction.reply({
            embeds: [createEmbed('Chave Nao Encontrada', 'Esta chave nao existe no sistema.', 0xef4444)],
            ephemeral: true,
          });
        }

        db.prepare(
          "UPDATE licenses SET revoked = 1, revoked_at = datetime('now'), revoked_by = ? WHERE key = ?"
        ).run(user.username, key);

        await interaction.reply({
          embeds: [createEmbed('Chave Revogada', `A chave \`${key}\` foi revogada com sucesso.`, 0xf59e0b)],
          ephemeral: true,
        });
        break;
      }

      case 'listar-keys': {
        if (!isAdmin(member)) {
          return interaction.reply({
            embeds: [createEmbed('Sem Permissao', 'Apenas administradores podem listar todas as chaves.', 0xef4444)],
            ephemeral: true,
          });
        }

        const keys = db.prepare(
          "SELECT * FROM licenses WHERE revoked = 0 AND expires_at > datetime('now') ORDER BY created_at DESC LIMIT 25"
        ).all();

        if (keys.length === 0) {
          return interaction.reply({
            embeds: [createEmbed('Nenhuma Chave', 'Nao ha chaves ativas no momento.', 0xf59e0b)],
            ephemeral: true,
          });
        }

        let description = '';
        for (const k of keys) {
          const expires = new Date(k.expires_at).toLocaleDateString('pt-BR');
          description += `\`${k.key}\` - **${k.discord_username}** (${k.tier}) - Exp: ${expires}\n`;
        }

        const totalActive = db.prepare(
          "SELECT COUNT(*) as count FROM licenses WHERE revoked = 0 AND expires_at > datetime('now')"
        ).get();

        const embed = createEmbed('Chaves Ativas', description, 0x00d4ff)
          .addFields({ name: 'Total Ativas', value: `${totalActive.count}`, inline: true });

        await interaction.reply({ embeds: [embed], ephemeral: true });
        break;
      }

      case 'minha-key': {
        const keys = db.prepare(
          "SELECT * FROM licenses WHERE discord_user_id = ? AND revoked = 0 AND expires_at > datetime('now') ORDER BY created_at DESC"
        ).all(user.id);

        if (keys.length === 0) {
          return interaction.reply({
            embeds: [createEmbed('Sem Chaves', 'Voce nao possui chaves ativas. Use `/gerar-key` para criar uma.', 0xf59e0b)],
            ephemeral: true,
          });
        }

        let description = '';
        for (const k of keys) {
          const expires = new Date(k.expires_at).toLocaleDateString('pt-BR');
          const daysLeft = Math.ceil((new Date(k.expires_at) - new Date()) / (1000 * 60 * 60 * 24));
          description += `\`\`\`${k.key}\`\`\`**Tier:** ${k.tier === 'pro' ? 'Pro' : 'Basic'} | **Expira:** ${expires} (${daysLeft} dias)\n\n`;
        }

        await interaction.reply({
          embeds: [createEmbed('Suas Chaves', description, 0x00d4ff)],
          ephemeral: true,
        });
        break;
      }

      case 'pixpbo-help': {
        const embed = createEmbed('PixPBO Protect - Comandos', '', 0x00d4ff)
          .addFields(
            { name: '/gerar-key [dias] [tier]', value: 'Gera uma nova chave de licenca', inline: false },
            { name: '/verificar-key [key]', value: 'Verifica o status de uma chave', inline: false },
            { name: '/minha-key', value: 'Mostra suas chaves ativas', inline: false },
            { name: '/revogar-key [key]', value: 'Revoga uma chave (admin)', inline: false },
            { name: '/listar-keys', value: 'Lista todas as chaves ativas (admin)', inline: false },
          )
          .setDescription('Sistema de licencas para o PixPBO Protect - Proteja seus mods DayZ!');

        await interaction.reply({ embeds: [embed] });
        break;
      }
    }
  } catch (error) {
    console.error('Erro no comando:', error);
    const reply = {
      embeds: [createEmbed('Erro', `Ocorreu um erro: ${error.message}`, 0xef4444)],
      ephemeral: true,
    };

    if (interaction.replied || interaction.deferred) {
      await interaction.followUp(reply);
    } else {
      await interaction.reply(reply);
    }
  }
});

// ---- Start Bot ----
if (config.token === 'YOUR_BOT_TOKEN_HERE') {
  console.log('\n' + '='.repeat(50));
  console.log('  PixPBO Protect - Discord Bot');
  console.log('  CONFIGURACAO NECESSARIA!');
  console.log('');
  console.log('  Edite o arquivo config.json com:');
  console.log('  - token: Token do bot Discord');
  console.log('  - clientId: ID do cliente/aplicacao');
  console.log('  - guildId: ID do servidor Discord');
  console.log('  - adminRoleId: ID do cargo de admin');
  console.log('='.repeat(50) + '\n');
  console.log('Crie um bot em: https://discord.com/developers/applications');
  process.exit(0);
} else {
  client.login(config.token);
}
