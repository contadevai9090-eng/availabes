// PIXStore - MissionServer (Servidor)
// Gerencia RPCs do lado servidor, comandos admin e inicializacao
modded class MissionServer
{
	// Lista de admins (SteamIDs)
	static ref array<string> m_PIXStoreAdmins;
	static string ADMIN_LIST_PATH = "$profile:PackFazupix\\PIXStore\\PIXStoreAdmins.txt";

	override void OnInit()
	{
		super.OnInit();

		// Criar diretorios necessarios
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");
		MakeDirectory("$profile:PackFazupix\\PIXStore\\bancoodedados");

		// Carregar lista de admins
		CarregarAdmins();

		// Registrar RPCs do servidor
		GetRPCManager().AddRPC("PIXStore", "RequestPlayerData", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AtivarSeguroRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RecuperarVeiculoRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RemoverSeguroRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RastrearVeiculoRPC", this, SingeplayerExecutionType.Server);

		// RPCs Admin (servidor recebe)
		GetRPCManager().AddRPC("PIXStore", "AdminRequestPlayerList", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminDarPixCoin", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminRemoverPixCoin", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminDarVIP", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminRemoverVIP", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminVerSaldo", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AdminVerVIP", this, SingeplayerExecutionType.Server);

		// Registrar RPCs de resposta (cliente)
		GetRPCManager().AddRPC("PIXStore", "ReceivePlayerData", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "UpdateMapMarkers", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "ReceiveStatusMessage", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "AdminReceivePlayerList", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "AdminReceiveResponse", this, SingeplayerExecutionType.Client);

		// Inicializar sistema
		PIXStoreSystemManager.Initialize();

		// Registrar comandos admin
		PIXStoreAdminCommands.RegisterChatCommands();

		// Inicializar rastreamento
		PIXStoreTracker.Initialize();

		// Timer para atualizar rastreamento (a cada 5 segundos)
		if (PIXStoreConfig.ATIVAR_RASTREAMENTO_MAPA)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AtualizarRastreamentoTodos, PIXStoreConfig.INTERVALO_ATUALIZACAO_BLIP_MS, true);
		}

		Print("[PIXStore] Sistema inicializado com sucesso!");
		Print("[PIXStore] Admins carregados: " + m_PIXStoreAdmins.Count().ToString());
	}

	// Carregar lista de admins do arquivo
	void CarregarAdmins()
	{
		m_PIXStoreAdmins = new array<string>();

		if (!FileExist(ADMIN_LIST_PATH))
		{
			// Criar arquivo de admins com instrucoes
			FileHandle file = OpenFile(ADMIN_LIST_PATH, FileMode.WRITE);
			if (file)
			{
				FPrintln(file, "// PIXStore - Lista de Admins");
				FPrintln(file, "// Coloque um SteamID64 por linha");
				FPrintln(file, "// Exemplo: 76561198251007370");
				CloseFile(file);
			}
			Print("[PIXStore] Arquivo de admins criado: " + ADMIN_LIST_PATH);
			return;
		}

		FileHandle fileRead = OpenFile(ADMIN_LIST_PATH, FileMode.READ);
		if (fileRead)
		{
			string line;
			while (FGets(fileRead, line) >= 0)
			{
				line = line.Trim();
				if (line.Length() > 0 && line.IndexOf("//") != 0)
				{
					m_PIXStoreAdmins.Insert(line);
					Print("[PIXStore] Admin carregado: " + line);
				}
			}
			CloseFile(fileRead);
		}
	}

	// Verificar se jogador e admin
	static bool IsAdmin(string steamId)
	{
		if (!m_PIXStoreAdmins)
			return false;

		for (int i = 0; i < m_PIXStoreAdmins.Count(); i++)
		{
			if (m_PIXStoreAdmins.Get(i) == steamId)
				return true;
		}
		return false;
	}

	// Registrar jogador quando conecta (cria JSON se nao existir)
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);

		if (identity)
		{
			string steamId = identity.GetPlainId();
			string playerName = identity.GetName();

			// Criar/atualizar dados do jogador no banco
			ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
			data.steamId = steamId;
			PIXStorePixCoin.SavePlayerData(steamId, data);

			PIXStoreLogManager.LogSistema("CONNECT", "Jogador conectou: " + playerName + " (" + steamId + ")");
			Print("[PIXStore] Jogador registrado no banco: " + playerName + " (" + steamId + ")");
		}
	}

	// RPC: Solicitar dados do jogador
	void RequestPlayerData(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<string> param;
			if (ctx.Read(param))
			{
				string steamId = param.param1;
				ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

				// Enviar dados para o cliente
				GetRPCManager().SendRPC("PIXStore", "ReceivePlayerData", new Param1<ref PIXStorePlayerData>(data), true, sender);
			}
		}
	}

	// RPC: Ativar seguro para veiculo atual do jogador
	void AtivarSeguroRPC(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
			if (!player) return;

			// Verificar se jogador esta em um veiculo
			HumanCommandVehicle cmdVehicle = player.GetCommand_Vehicle();
			if (!cmdVehicle)
			{
				player.MessageStatus("[PIXStore] Voce precisa estar em um veiculo!");
				return;
			}

			Transport transport = cmdVehicle.GetTransport();
			if (!transport)
			{
				player.MessageStatus("[PIXStore] Veiculo invalido!");
				return;
			}

			EntityAI veiculoEntity = EntityAI.Cast(transport);
			if (!veiculoEntity)
			{
				player.MessageStatus("[PIXStore] Erro ao identificar veiculo!");
				return;
			}

			// Ativar seguro
			bool sucesso = PIXStoreSystemManager.AtivarSeguro(player, veiculoEntity);

			if (sucesso)
			{
				// Enviar dados atualizados
				string steamId = sender.GetPlainId();
				ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
				GetRPCManager().SendRPC("PIXStore", "ReceivePlayerData", new Param1<ref PIXStorePlayerData>(data), true, sender);
			}
		}
	}

	// RPC: Recuperar veiculo
	void RecuperarVeiculoRPC(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<int> param;
			if (ctx.Read(param))
			{
				PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
				if (!player) return;

				bool sucesso = PIXStoreRecovery.RecuperarVeiculo(player, param.param1);

				// Enviar dados atualizados
				string steamId = sender.GetPlainId();
				ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
				GetRPCManager().SendRPC("PIXStore", "ReceivePlayerData", new Param1<ref PIXStorePlayerData>(data), true, sender);
			}
		}
	}

	// RPC: Remover seguro
	void RemoverSeguroRPC(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<int> param;
			if (ctx.Read(param))
			{
				PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
				if (!player) return;

				PIXStoreSystemManager.RemoverSeguro(player, param.param1);

				// Enviar dados atualizados
				string steamId = sender.GetPlainId();
				ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
				GetRPCManager().SendRPC("PIXStore", "ReceivePlayerData", new Param1<ref PIXStorePlayerData>(data), true, sender);
			}
		}
	}

	// RPC: Rastrear veiculo
	void RastrearVeiculoRPC(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			Param1<int> param;
			if (ctx.Read(param))
			{
				PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
				if (!player) return;

				string steamId = sender.GetPlainId();

				// Iniciar rastreamento
				PIXStoreTracker.IniciarRastreamento(steamId);

				// Atualizar posicoes e enviar marcadores
				PIXStoreTracker.AtualizarMarcadoresBasicMap(player, steamId);
			}
		}
	}

	// ===================== ADMIN RPCs =====================

	// Admin: Solicitar lista de jogadores online
	void AdminRequestPlayerList(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			// Montar lista: nome|steamid|saldo|vip;nome|steamid|saldo|vip;...
			string result = "";
			ref array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);

			for (int i = 0; i < players.Count(); i++)
			{
				PlayerBase player = PlayerBase.Cast(players.Get(i));
				if (player && player.GetIdentity())
				{
					string steamId = player.GetIdentity().GetPlainId();
					string nome = player.GetIdentity().GetName();
					int saldo = PIXStorePixCoin.GetSaldo(steamId);
					string vipStatus = "Nao";
					if (PIXStoreVIP.IsVIP(steamId))
						vipStatus = "Sim";

					if (result.Length() > 0)
						result = result + ";";

					result = result + nome + "|" + steamId + "|" + saldo.ToString() + "|" + vipStatus;
				}
			}

			GetRPCManager().SendRPC("PIXStore", "AdminReceivePlayerList", new Param1<string>(result), true, sender);
		}
	}

	// Admin: Dar PixCoin
	void AdminDarPixCoin(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param2<string, int> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				int valor = param.param2;

				PIXStorePixCoin.Creditar(targetSteamId, valor, adminId);
				int novoSaldo = PIXStorePixCoin.GetSaldo(targetSteamId);

				string msg = "[OK] +" + valor.ToString() + " PixCoin para " + targetSteamId + " | Saldo: " + novoSaldo.ToString();
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);

				// Notificar jogador alvo
				PlayerBase targetPlayer = PIXStoreAdminCommands.GetJogadorBySteamId(targetSteamId);
				if (targetPlayer)
					targetPlayer.MessageStatus("[PIXStore] Voce recebeu " + valor.ToString() + " PixCoin!");

				PIXStoreLogManager.LogAdmin(adminId, "PAINEL_DAR_PIXCOIN", targetSteamId, "Valor: " + valor.ToString());
			}
		}
	}

	// Admin: Remover PixCoin
	void AdminRemoverPixCoin(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param2<string, int> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				int valor = param.param2;

				bool sucesso = PIXStorePixCoin.Debitar(targetSteamId, valor, "Admin removeu");
				int novoSaldo = PIXStorePixCoin.GetSaldo(targetSteamId);

				string msg;
				if (sucesso)
					msg = "[OK] -" + valor.ToString() + " PixCoin de " + targetSteamId + " | Saldo: " + novoSaldo.ToString();
				else
					msg = "[ERRO] Saldo insuficiente! Saldo atual: " + novoSaldo.ToString();

				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);
				PIXStoreLogManager.LogAdmin(adminId, "PAINEL_REMOVER_PIXCOIN", targetSteamId, "Valor: " + valor.ToString());
			}
		}
	}

	// Admin: Dar VIP
	void AdminDarVIP(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param2<string, int> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				int dias = param.param2;

				if (PIXStoreVIP.IsVIP(targetSteamId))
					PIXStoreVIP.EstenderVIP(targetSteamId, dias, adminId);
				else
					PIXStoreVIP.AtivarVIP(targetSteamId, dias, adminId);

				string msg = "[OK] VIP " + dias.ToString() + " dias para " + targetSteamId;
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);

				PlayerBase targetPlayer = PIXStoreAdminCommands.GetJogadorBySteamId(targetSteamId);
				if (targetPlayer)
					targetPlayer.MessageStatus("[PIXStore] Voce recebeu VIP por " + dias.ToString() + " dias!");

				PIXStoreLogManager.LogAdmin(adminId, "PAINEL_DAR_VIP", targetSteamId, "Dias: " + dias.ToString());
			}
		}
	}

	// Admin: Remover VIP
	void AdminRemoverVIP(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param1<string> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				PIXStoreVIP.RevogarVIP(targetSteamId, adminId);

				string msg = "[OK] VIP removido de " + targetSteamId;
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);
				PIXStoreLogManager.LogAdmin(adminId, "PAINEL_REMOVER_VIP", targetSteamId);
			}
		}
	}

	// Admin: Ver saldo
	void AdminVerSaldo(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param1<string> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				int saldo = PIXStorePixCoin.GetSaldo(targetSteamId);

				string msg = "Saldo de " + targetSteamId + ": " + saldo.ToString() + " PixCoin";
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);
			}
		}
	}

	// Admin: Ver VIP restante
	void AdminVerVIP(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Server && sender)
		{
			string adminId = sender.GetPlainId();
			if (!IsAdmin(adminId))
			{
				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>("[ERRO] Voce nao e admin!"), true, sender);
				return;
			}

			Param1<string> param;
			if (ctx.Read(param))
			{
				string targetSteamId = param.param1;
				bool isVIP = PIXStoreVIP.IsVIP(targetSteamId);
				string tempoRestante = PIXStoreVIP.GetTempoRestanteFormatado(targetSteamId);

				string msg;
				if (isVIP)
					msg = "VIP de " + targetSteamId + " | Restante: " + tempoRestante;
				else
					msg = targetSteamId + " NAO possui VIP";

				GetRPCManager().SendRPC("PIXStore", "AdminReceiveResponse", new Param1<string>(msg), true, sender);
			}
		}
	}

	// Processar mensagens de chat (para comandos admin)
	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);

		if (eventTypeId != ChatMessageEventTypeID)
			return;

		ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
		if (!chatParams)
			return;

		string senderName = chatParams.param2;
		string mensagem = chatParams.param3;

		if (mensagem.Length() == 0)
			return;

		// Verificar se e um comando PIXStore
		if (mensagem.IndexOf("/darpixcoin") != 0 && mensagem.IndexOf("/viprestante") != 0 && mensagem.IndexOf("/darvip") != 0 && mensagem.IndexOf("/removervip") != 0 && mensagem.IndexOf("/pixsaldo") != 0)
			return;

		// Encontrar a identidade do jogador que enviou
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		for (int i = 0; i < players.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(players.Get(i));
			if (!player || !player.GetIdentity())
				continue;

			string playerName = player.GetIdentity().GetName();
			if (playerName == senderName)
			{
				PIXStoreAdminCommands.ProcessarComando(player.GetIdentity(), mensagem);
				break;
			}
		}
	}

	// Atualizar rastreamento para todos os jogadores conectados
	void AtualizarRastreamentoTodos()
	{
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity())
			{
				string steamId = player.GetIdentity().GetPlainId();
				PIXStoreTracker.AtualizarMarcadoresBasicMap(player, steamId);
			}
		}
	}
}
