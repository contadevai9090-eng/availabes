// PIXStore - Sistema de Rastreamento em Tempo Real
// Integra com BasicMap para mostrar blips dos veiculos segurados
class PIXStoreTracker
{
	static ref map<string, ref array<ref PIXStoreBlipData>> m_PlayerBlips;
	static bool m_Initialized = false;

	static void Initialize()
	{
		if (m_Initialized) return;
		m_Initialized = true;
		m_PlayerBlips = new map<string, ref array<ref PIXStoreBlipData>>();
	}

	// Iniciar rastreamento para um jogador
	static void IniciarRastreamento(string steamId)
	{
		if (!PIXStoreConfig.ATIVAR_RASTREAMENTO_MAPA)
			return;

		Initialize();

		// Remover blips antigos deste jogador
		RemoverTodosBlips(steamId);

		// Obter veiculos segurados
		ref array<ref PIXStoreVeiculoData> veiculos = PIXStoreSystemManager.GetVeiculosSegurados(steamId);

		if (!veiculos || veiculos.Count() == 0)
			return;

		ref array<ref PIXStoreBlipData> blips = new array<ref PIXStoreBlipData>();

		foreach (PIXStoreVeiculoData veiculo : veiculos)
		{
			ref PIXStoreBlipData blip = new PIXStoreBlipData();
			blip.veiculoUID = veiculo.veiculoUID;
			blip.classname = veiculo.classname;
			blip.displayName = veiculo.displayName;
			blip.posicao = Vector(0, 0, 0);
			blip.ativo = true;
			blips.Insert(blip);
		}

		m_PlayerBlips.Set(steamId, blips);

		PIXStoreLogManager.LogActivity(steamId, "RASTREAMENTO", "Iniciado para " + veiculos.Count().ToString() + " veiculos");
	}

	// Atualizar posicoes dos veiculos rastreados (chamada periodica no cliente)
	static void AtualizarPosicoes(string steamId)
	{
		if (!PIXStoreConfig.ATIVAR_RASTREAMENTO_MAPA)
			return;

		Initialize();

		if (!m_PlayerBlips.Contains(steamId))
			return;

		ref array<ref PIXStoreBlipData> blips = m_PlayerBlips.Get(steamId);

		for (int i = blips.Count() - 1; i >= 0; i--)
		{
			PIXStoreBlipData blip = blips.Get(i);

			// Localizar veiculo no mundo
			EntityAI veiculo = PIXStoreAntiTheft.LocalizarVeiculoPorUID(blip.veiculoUID, blip.classname);

			if (veiculo)
			{
				// Atualizar posicao
				blip.posicao = veiculo.GetPosition();
				blip.ativo = true;
			}
			else
			{
				// Veiculo nao existe mais, remover blip
				blip.ativo = false;
				blips.Remove(i);
			}
		}
	}

	// Obter dados de blips para enviar ao cliente (BasicMap integration)
	static ref array<ref PIXStoreBlipData> GetBlipsJogador(string steamId)
	{
		Initialize();

		if (!m_PlayerBlips.Contains(steamId))
			return new array<ref PIXStoreBlipData>();

		return m_PlayerBlips.Get(steamId);
	}

	// Remover todos os blips de um jogador
	static void RemoverTodosBlips(string steamId)
	{
		Initialize();

		if (m_PlayerBlips.Contains(steamId))
		{
			m_PlayerBlips.Remove(steamId);
		}
	}

	// Obter posicao de um veiculo especifico
	static vector GetPosicaoVeiculo(string steamId, int veiculoIndex)
	{
		Initialize();

		if (!m_PlayerBlips.Contains(steamId))
			return Vector(0, 0, 0);

		ref array<ref PIXStoreBlipData> blips = m_PlayerBlips.Get(steamId);

		if (veiculoIndex < 0 || veiculoIndex >= blips.Count())
			return Vector(0, 0, 0);

		return blips.Get(veiculoIndex).posicao;
	}

	// Criar marcador no mapa do BasicMap
	static void CriarMarcadorBasicMap(PlayerBase player, PIXStoreBlipData blip)
	{
		if (!player || !blip.ativo)
			return;

		// BasicMap integration: enviar dados via RPC para criar marcador
		// O cliente usara a API do BasicMap para criar o marcador
		string markerData = blip.displayName + "|" + blip.posicao[0].ToString() + "|" + blip.posicao[2].ToString();
		GetRPCManager().SendRPC("PIXStore", "CreateMapMarker", new Param1<string>(markerData), true, player.GetIdentity());
	}

	// Atualizar marcadores de um jogador no BasicMap
	static void AtualizarMarcadoresBasicMap(PlayerBase player, string steamId)
	{
		if (!player || !PIXStoreConfig.ATIVAR_RASTREAMENTO_MAPA)
			return;

		// Primeiro atualizar posicoes
		AtualizarPosicoes(steamId);

		// Obter blips ativos
		ref array<ref PIXStoreBlipData> blips = GetBlipsJogador(steamId);

		// Enviar dados serializados de todos os blips
		string allMarkers = "";
		foreach (PIXStoreBlipData blip : blips)
		{
			if (blip.ativo)
			{
				if (allMarkers != "")
					allMarkers += ";";
				allMarkers += blip.displayName + "|" + blip.posicao[0].ToString() + "|" + blip.posicao[2].ToString();
			}
		}

		if (allMarkers != "")
		{
			GetRPCManager().SendRPC("PIXStore", "UpdateMapMarkers", new Param1<string>(allMarkers), true, player.GetIdentity());
		}
	}
}

// Dados de blip para rastreamento
class PIXStoreBlipData
{
	string veiculoUID;
	string classname;
	string displayName;
	vector posicao;
	bool ativo;

	void PIXStoreBlipData()
	{
		ativo = false;
		posicao = Vector(0, 0, 0);
	}
}
