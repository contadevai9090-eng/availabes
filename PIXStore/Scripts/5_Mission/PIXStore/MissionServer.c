// PIXStore - MissionServer (Servidor)
// Gerencia RPCs do lado servidor, comandos admin e inicializacao
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();

		// Criar diretorios necessarios
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");
		MakeDirectory("$profile:PackFazupix\\PIXStore\\bancoodedados");

		// Registrar RPCs do servidor
		GetRPCManager().AddRPC("PIXStore", "RequestPlayerData", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "AtivarSeguroRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RecuperarVeiculoRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RemoverSeguroRPC", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("PIXStore", "RastrearVeiculoRPC", this, SingeplayerExecutionType.Server);

		// Registrar RPCs de resposta (cliente)
		GetRPCManager().AddRPC("PIXStore", "ReceivePlayerData", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "UpdateMapMarkers", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "ReceiveStatusMessage", this, SingeplayerExecutionType.Client);

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

	// Processar mensagens de chat (para comandos admin)
	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);

		if (eventTypeId == ChatMessageEventTypeID)
		{
			ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
			if (chatParams)
			{
				string mensagem = chatParams.param2;

				// Verificar se e um comando PIXStore
				if (mensagem.IndexOf("/darpixcoin") == 0 || mensagem.IndexOf("/viprestante") == 0 || mensagem.IndexOf("/darvip") == 0 || mensagem.IndexOf("/removervip") == 0 || mensagem.IndexOf("/pixsaldo") == 0)
				{
					// Encontrar a identidade do jogador que enviou
					ref array<Man> players = new array<Man>;
					GetGame().GetPlayers(players);

					foreach (Man man : players)
					{
						PlayerBase player = PlayerBase.Cast(man);
						if (player && player.GetIdentity() && player.GetIdentity().GetName() == chatParams.param1)
						{
							PIXStoreAdminCommands.ProcessarComando(player.GetIdentity(), mensagem);
							break;
						}
					}
				}
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
