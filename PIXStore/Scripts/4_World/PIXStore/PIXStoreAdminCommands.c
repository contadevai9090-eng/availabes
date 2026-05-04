// PIXStore - Comandos de Admin
// /darpixcoin [jogador] [valor] - Adicionar PixCoin
// /viprestante [jogador] - Verificar tempo restante do VIP
class PIXStoreAdminCommands
{
	// Registrar handlers de chat
	static void RegisterChatCommands()
	{
		// Os comandos serao processados no OnChatMessage do MissionServer
		PIXStoreLogManager.LogSistema("ADMIN", "Comandos de admin registrados");
	}

	// Processar comando de chat
	static bool ProcessarComando(PlayerIdentity sender, string mensagem)
	{
		if (!sender || mensagem == "")
			return false;

		// Verificar se e admin (via lista de admins ou permissao)
		string adminId = sender.GetPlainId();

		// /darpixcoin [steamid/nome] [valor]
		if (mensagem.IndexOf("/darpixcoin") == 0)
		{
			return CmdDarPixCoin(sender, mensagem, adminId);
		}

		// /viprestante [steamid/nome]
		if (mensagem.IndexOf("/viprestante") == 0)
		{
			return CmdVIPRestante(sender, mensagem, adminId);
		}

		// /darvip [steamid/nome] [dias]
		if (mensagem.IndexOf("/darvip") == 0)
		{
			return CmdDarVIP(sender, mensagem, adminId);
		}

		// /removervip [steamid/nome]
		if (mensagem.IndexOf("/removervip") == 0)
		{
			return CmdRemoverVIP(sender, mensagem, adminId);
		}

		// /pixsaldo [steamid/nome]
		if (mensagem.IndexOf("/pixsaldo") == 0)
		{
			return CmdPixSaldo(sender, mensagem, adminId);
		}

		return false;
	}

	// Comando: /darpixcoin [jogador] [valor]
	static bool CmdDarPixCoin(PlayerIdentity sender, string mensagem, string adminId)
	{
		// Parsear argumentos
		ref array<string> args = new array<string>();
		mensagem.Split(" ", args);

		if (args.Count() < 3)
		{
			EnviarMensagemAdmin(sender, "[PIXStore] Uso: /darpixcoin [steamid] [valor]");
			return true;
		}

		string targetId = args.Get(1);
		int valor = args.Get(2).ToInt();

		if (valor <= 0)
		{
			EnviarMensagemAdmin(sender, "[PIXStore] Valor deve ser positivo");
			return true;
		}

		// Verificar se jogador existe (tentar encontrar pelo steamid ou nome)
		string steamId = ResolverJogador(targetId);
		if (steamId == "")
		{
			// Usar como steamid direto
			steamId = targetId;
		}

		// Creditar PixCoin
		PIXStorePixCoin.Creditar(steamId, valor, adminId);

		int novoSaldo = PIXStorePixCoin.GetSaldo(steamId);
		EnviarMensagemAdmin(sender, "[PIXStore] " + valor.ToString() + " PixCoin creditado para " + steamId + " | Novo saldo: " + novoSaldo.ToString());

		// Notificar jogador alvo se estiver online
		PlayerBase targetPlayer = GetJogadorBySteamId(steamId);
		if (targetPlayer)
		{
			targetPlayer.MessageStatus("[PIXStore] Voce recebeu " + valor.ToString() + " PixCoin! Saldo: " + novoSaldo.ToString());
		}

		PIXStoreLogManager.LogAdmin(adminId, "DAR_PIXCOIN", steamId, "Valor: " + valor.ToString());
		return true;
	}

	// Comando: /viprestante [jogador]
	static bool CmdVIPRestante(PlayerIdentity sender, string mensagem, string adminId)
	{
		ref array<string> args = new array<string>();
		mensagem.Split(" ", args);

		string steamId;

		if (args.Count() < 2)
		{
			// Verificar para si mesmo
			steamId = adminId;
		}
		else
		{
			string targetId = args.Get(1);
			steamId = ResolverJogador(targetId);
			if (steamId == "")
				steamId = targetId;
		}

		bool isVIP = PIXStoreVIP.IsVIP(steamId);
		string tempoRestante = PIXStoreVIP.GetTempoRestanteFormatado(steamId);
		string dataExp = PIXStoreVIP.GetDataExpiracao(steamId);

		string msg;
		if (isVIP)
		{
			msg = "[PIXStore] VIP de " + steamId + " | Tempo restante: " + tempoRestante + " | Expira: " + dataExp;
		}
		else
		{
			msg = "[PIXStore] " + steamId + " NAO possui VIP ativo";
		}

		EnviarMensagemAdmin(sender, msg);
		return true;
	}

	// Comando: /darvip [jogador] [dias]
	static bool CmdDarVIP(PlayerIdentity sender, string mensagem, string adminId)
	{
		ref array<string> args = new array<string>();
		mensagem.Split(" ", args);

		if (args.Count() < 3)
		{
			EnviarMensagemAdmin(sender, "[PIXStore] Uso: /darvip [steamid] [dias]");
			return true;
		}

		string targetId = args.Get(1);
		int dias = args.Get(2).ToInt();

		if (dias <= 0)
		{
			EnviarMensagemAdmin(sender, "[PIXStore] Dias deve ser positivo");
			return true;
		}

		string steamId = ResolverJogador(targetId);
		if (steamId == "")
			steamId = targetId;

		// Ativar ou estender VIP
		if (PIXStoreVIP.IsVIP(steamId))
		{
			PIXStoreVIP.EstenderVIP(steamId, dias, adminId);
			EnviarMensagemAdmin(sender, "[PIXStore] VIP estendido por " + dias.ToString() + " dias para " + steamId);
		}
		else
		{
			PIXStoreVIP.AtivarVIP(steamId, dias, adminId);
			EnviarMensagemAdmin(sender, "[PIXStore] VIP ativado por " + dias.ToString() + " dias para " + steamId);
		}

		// Notificar jogador alvo
		PlayerBase targetPlayer = GetJogadorBySteamId(steamId);
		if (targetPlayer)
		{
			targetPlayer.MessageStatus("[PIXStore] Voce recebeu VIP por " + dias.ToString() + " dias!");
		}

		PIXStoreLogManager.LogAdmin(adminId, "DAR_VIP", steamId, "Dias: " + dias.ToString());
		return true;
	}

	// Comando: /removervip [jogador]
	static bool CmdRemoverVIP(PlayerIdentity sender, string mensagem, string adminId)
	{
		ref array<string> args = new array<string>();
		mensagem.Split(" ", args);

		if (args.Count() < 2)
		{
			EnviarMensagemAdmin(sender, "[PIXStore] Uso: /removervip [steamid]");
			return true;
		}

		string targetId = args.Get(1);
		string steamId = ResolverJogador(targetId);
		if (steamId == "")
			steamId = targetId;

		PIXStoreVIP.RevogarVIP(steamId, adminId);
		EnviarMensagemAdmin(sender, "[PIXStore] VIP removido de " + steamId);

		PIXStoreLogManager.LogAdmin(adminId, "REMOVER_VIP", steamId);
		return true;
	}

	// Comando: /pixsaldo [jogador]
	static bool CmdPixSaldo(PlayerIdentity sender, string mensagem, string adminId)
	{
		ref array<string> args = new array<string>();
		mensagem.Split(" ", args);

		string steamId;

		if (args.Count() < 2)
		{
			steamId = adminId;
		}
		else
		{
			string targetId = args.Get(1);
			steamId = ResolverJogador(targetId);
			if (steamId == "")
				steamId = targetId;
		}

		int saldo = PIXStorePixCoin.GetSaldo(steamId);
		EnviarMensagemAdmin(sender, "[PIXStore] Saldo de " + steamId + ": " + saldo.ToString() + " PixCoin");
		return true;
	}

	// Resolver jogador por nome ou steamid
	static string ResolverJogador(string identificador)
	{
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity())
			{
				// Verificar por steamid
				if (player.GetIdentity().GetPlainId() == identificador)
					return identificador;

				// Verificar por nome (parcial)
				string playerName = player.GetIdentity().GetName();
				playerName.ToLower();
				string searchName = identificador;
				searchName.ToLower();

				if (playerName.IndexOf(searchName) >= 0)
					return player.GetIdentity().GetPlainId();
			}
		}

		return "";
	}

	// Obter jogador pelo steamid
	static PlayerBase GetJogadorBySteamId(string steamId)
	{
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == steamId)
				return player;
		}

		return null;
	}

	// Enviar mensagem para admin
	static void EnviarMensagemAdmin(PlayerIdentity sender, string msg)
	{
		PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
		if (player)
		{
			player.MessageStatus(msg);
		}
	}
}
