# PIXSupport Bot

> Bot Discord de atendimento automatizado com tickets, integracao DayZ e doacoes PIX.

## Funcionalidades

- **Sistema de Tickets** com modal (pop-up) e embeds profissionais
  - Categorias: Doacao, Suporte, Duvida
  - Canal privado automatico com permissoes
  - Botoes: Fechar Ticket, Assumir Ticket
  - DM automatica ao abrir/fechar ticket
  - Transcript/log ao fechar
- **Integracao com Doacoes PIX**
  - Ticket especial com chave PIX e instrucoes
- **Sistema de Staff**
  - Cargo staff pode responder, fechar e assumir tickets
- **Anti-Spam**
  - Cooldown por usuario
  - Bloqueio de multiplos tickets simultaneos
- **Integracao DayZ**
  - `/servidor` - Status online/offline, jogadores, IP, botao de conexao
  - `/topkills` - Ranking de kills
  - `/addkill` - Registrar kills (staff)
  - `/vip` - Ativar VIP com cargo automatico

## Requisitos

- Node.js 18+
- Bot Discord criado em https://discord.com/developers/applications

## Instalacao

```bash
cd pixsupport-bot
npm install
```

## Configuracao

Edite `config.json`:

```json
{
  "token": "SEU_TOKEN_AQUI",
  "clientId": "ID_DO_CLIENTE",
  "guildId": "ID_DO_SERVIDOR",
  "logChannelId": "ID_CANAL_LOGS",
  "ticketCategoryId": "ID_CATEGORIA_TICKET",
  "staffRoleId": "ID_CARGO_STAFF",
  "donationLink": "SUA_CHAVE_PIX_OU_LINK",
  "donationInstructions": "Envie o comprovante neste ticket...",
  "serverLogo": "https://i.imgur.com/XXXXXXX.png",
  "serverName": "BR Terra Sem Lei",
  "embedColor": "#00FF88",
  "cooldownSeconds": 60,
  "dayz": {
    "serverIp": "0.0.0.0",
    "serverPort": 2302,
    "queryPort": 27016,
    "connectUrl": "steam://connect/{ip}:{port}",
    "vipRoleId": "ID_CARGO_VIP",
    "topKillsChannelId": "ID_CANAL_TOP_KILLS"
  }
}
```

### Campos

| Campo | Descricao |
|-------|-----------|
| `token` | Token do bot Discord |
| `clientId` | ID do cliente/aplicacao |
| `guildId` | ID do servidor Discord |
| `logChannelId` | Canal para logs de tickets |
| `ticketCategoryId` | Categoria onde tickets serao criados |
| `staffRoleId` | Cargo da equipe de atendimento |
| `donationLink` | Chave PIX ou link de pagamento |
| `serverLogo` | URL da logo do servidor |
| `serverName` | Nome exibido nos embeds |
| `embedColor` | Cor dos embeds (hex) |
| `cooldownSeconds` | Tempo entre tickets por usuario |
| `dayz.serverIp` | IP do servidor DayZ |
| `dayz.serverPort` | Porta do servidor |
| `dayz.queryPort` | Porta de query |
| `dayz.vipRoleId` | Cargo VIP no Discord |

## Permissoes do Bot

O bot precisa das seguintes permissoes no Discord:

- Manage Channels
- Manage Roles
- Send Messages
- Embed Links
- Attach Files
- Read Message History
- Use Application Commands

**Intents obrigatorios:**

- Server Members Intent
- Message Content Intent

## Uso

### Registrar comandos

```bash
npm run register
```

### Iniciar o bot

```bash
npm start
```

### Manter online 24/7

```bash
npm install -g pm2
pm2 start index.js --name "pixsupport"
pm2 save
pm2 startup
```

## Comandos

| Comando | Descricao | Permissao |
|---------|-----------|-----------|
| `/ticket` | Envia painel de tickets | Gerenciar Servidor |
| `/servidor` | Status do servidor DayZ | Todos |
| `/topkills` | Ranking de kills | Todos |
| `/addkill [jogador]` | Registra kills | Staff |
| `/vip [usuario] [dias]` | Ativa VIP | Gerenciar Cargos |

## Estrutura

```
pixsupport-bot/
├── index.js              # Entry point
├── config.json           # Configuracoes
├── package.json          # Dependencias
├── commands/
│   ├── ticket.js         # Painel de tickets
│   ├── servidor.js       # Status DayZ
│   ├── topkills.js       # Ranking kills
│   ├── vip.js            # Ativar VIP
│   └── addkill.js        # Registrar kill
├── events/
│   ├── ready.js          # Evento de inicializacao
│   └── interactionCreate.js  # Handler de interacoes
└── utils/
    ├── embeds.js          # Embeds e botoes
    ├── transcript.js      # Exportar conversa
    ├── cooldown.js        # Anti-spam
    ├── dayz.js            # Integracao DayZ
    └── register-commands.js  # Registrar slash commands
```
