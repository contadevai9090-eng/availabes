// PIXStore - Gerenciador Principal do Sistema
// Controla registro de veiculos, verificacoes e persistencia
class PIXStoreSystemManager
{
	// Inicializar sistema
	static void Initialize()
	{
		// Criar diretorios
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");
		MakeDirectory("$profile:PackFazupix\\PIXStore\\bancoodedados");

		// Carregar configuracao
		PIXStoreConfig.LoadConfig();

		// Inicializar log
		PIXStoreLogManager.Init();

		// Iniciar limpeza periodica
		if (PIXStoreConfig.ENABLE_AUTO_CLEANUP)
			StartPeriodicCleanup();

		// Iniciar checagem de VIPs expirados (a cada 5 minutos)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PIXStoreVIP.ChecarVIPsExpirados, 300000, true);

		PIXStoreLogManager.LogSistema("INICIALIZADO", "Sistema PIXStore inicializado com sucesso");
	}

	// Ativar seguro para veiculo do jogador
	static bool AtivarSeguro(PlayerBase player, EntityAI veiculo)
	{
		if (!player || !veiculo)
			return false;

		string steamId = player.GetIdentity().GetPlainId();

		// Anti-spam
		if (!VerificarAntiSpam(steamId))
		{
			player.MessageStatus("[PIXStore] Aguarde antes de usar novamente");
			PIXStoreLogManager.LogAbuso(steamId, "SPAM", "Tentativa de spam no ativar seguro");
			return false;
		}

		// Verificar se e um veiculo
		CarScript car = CarScript.Cast(veiculo);
		if (!car)
		{
			player.MessageStatus("[PIXStore] Apenas veiculos podem ser segurados");
			return false;
		}

		// Verificar saldo de PixCoin
		int custo = PIXStoreConfig.CUSTO_ATIVAR_SEGURO;
		if (!PIXStorePixCoin.TemSaldo(steamId, custo))
		{
			player.MessageStatus("[PIXStore] Saldo insuficiente! Necessario: " + custo.ToString() + " PixCoin");
			PIXStoreLogManager.LogFalha(steamId, "ATIVAR_SEGURO", "Saldo insuficiente: " + PIXStorePixCoin.GetSaldo(steamId).ToString() + "/" + custo.ToString());
			return false;
		}

		// Verificar limite de veiculos
		bool isVIP = PIXStoreVIP.IsVIP(steamId);
		int maxVeiculos = PIXStoreConfig.GetMaxVeiculos(isVIP);
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		// Contar veiculos ativos
		int veiculosAtivos = 0;
		foreach (PIXStoreVeiculoData v : data.veiculos)
		{
			if (v.seguroAtivo)
				veiculosAtivos++;
		}

		if (veiculosAtivos >= maxVeiculos)
		{
			player.MessageStatus("[PIXStore] Limite de veiculos atingido: " + maxVeiculos.ToString());
			return false;
		}

		// Verificar se veiculo ja esta segurado
		string veiculoUID = GetVeiculoUID(veiculo);
		foreach (PIXStoreVeiculoData existente : data.veiculos)
		{
			if (existente.veiculoUID == veiculoUID && existente.seguroAtivo)
			{
				player.MessageStatus("[PIXStore] Este veiculo ja esta segurado");
				return false;
			}
		}

		// Debitar PixCoin
		if (!PIXStorePixCoin.Debitar(steamId, custo, "Ativar seguro - " + veiculo.GetDisplayName()))
		{
			player.MessageStatus("[PIXStore] Erro ao debitar PixCoin");
			return false;
		}

		// Registrar veiculo
		ref PIXStoreVeiculoData novoVeiculo = new PIXStoreVeiculoData();
		novoVeiculo.classname = veiculo.GetType();
		novoVeiculo.displayName = veiculo.GetDisplayName();
		novoVeiculo.veiculoUID = veiculoUID;
		novoVeiculo.posicao = veiculo.GetPosition().ToString();
		novoVeiculo.seguroAtivo = true;

		// Data de registro
		CF_Date now = CF_Date.Now();
		novoVeiculo.dataRegistro = now.Format(CF_Date.DATETIME);

		// Data de expiracao (usando dias VIP ou padrao)
		int diasExpiracao = PIXStoreConfig.VIP_DURACAO_DIAS;
		CF_Date expiracao = CF_Date.Now();
		int expTimestamp = expiracao.GetTimestamp() + (diasExpiracao * 86400);
		expiracao.EpochToDate(expTimestamp);
		novoVeiculo.dataExpiracao = expiracao.Format(CF_Date.DATETIME);

		// Registrar attachments
		RegistrarAttachments(car, novoVeiculo);

		data.veiculos.Insert(novoVeiculo);
		PIXStorePixCoin.SavePlayerData(steamId, data);

		PIXStoreLogManager.LogCompraSeguro(steamId, novoVeiculo.displayName, custo);
		PIXStoreLogManager.LogAtivacaoSeguro(steamId, novoVeiculo.displayName, veiculoUID);

		player.MessageStatus("[PIXStore] Seguro ativado para: " + novoVeiculo.displayName + " | Custo: " + custo.ToString() + " PixCoin");
		return true;
	}

	// Obter UID unico do veiculo
	static string GetVeiculoUID(EntityAI veiculo)
	{
		// Usar combinacao de tipo + posicao inicial como UID
		int netId1, netId2;
		veiculo.GetNetworkID(netId1, netId2);
		return veiculo.GetType() + "_" + netId1.ToString() + "_" + netId2.ToString();
	}

	// Registrar attachments do veiculo
	static void RegistrarAttachments(CarScript car, ref PIXStoreVeiculoData veiculoData)
	{
		for (int i = 0; i < car.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = car.GetInventory().GetAttachmentFromIndex(i);
			if (attachment)
			{
				veiculoData.attachments.Insert(attachment.GetType());
			}
		}
	}

	// Obter lista de veiculos segurados do jogador
	static ref array<ref PIXStoreVeiculoData> GetVeiculosSegurados(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
		ref array<ref PIXStoreVeiculoData> ativos = new array<ref PIXStoreVeiculoData>();

		foreach (PIXStoreVeiculoData v : data.veiculos)
		{
			if (v.seguroAtivo)
				ativos.Insert(v);
		}

		return ativos;
	}

	// Remover seguro de veiculo
	static void RemoverSeguro(PlayerBase player, int veiculoIndex)
	{
		if (!player) return;

		string steamId = player.GetIdentity().GetPlainId();
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		// Obter apenas veiculos ativos
		ref array<int> indicesAtivos = new array<int>();
		for (int i = 0; i < data.veiculos.Count(); i++)
		{
			if (data.veiculos.Get(i).seguroAtivo)
				indicesAtivos.Insert(i);
		}

		if (veiculoIndex < 0 || veiculoIndex >= indicesAtivos.Count())
		{
			player.MessageStatus("[PIXStore] Veiculo invalido");
			return;
		}

		int realIndex = indicesAtivos.Get(veiculoIndex);
		PIXStoreVeiculoData veiculo = data.veiculos.Get(realIndex);
		string nomeVeiculo = veiculo.displayName;

		// Desativar seguro
		veiculo.seguroAtivo = false;
		PIXStorePixCoin.SavePlayerData(steamId, data);

		PIXStoreLogManager.LogActivity(steamId, "REMOVER_SEGURO", nomeVeiculo);
		player.MessageStatus("[PIXStore] Seguro removido: " + nomeVeiculo);
	}

	// Verificar anti-spam
	static bool VerificarAntiSpam(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
		int now = GetGame().GetTime();

		if (data.ultimoComandoTimestamp > 0)
		{
			int diff = now - data.ultimoComandoTimestamp;
			if (diff < (PIXStoreConfig.ANTI_SPAM_COOLDOWN_SECONDS * 1000))
				return false;
		}

		data.ultimoComandoTimestamp = now;
		PIXStorePixCoin.SavePlayerData(steamId, data);
		return true;
	}

	// Limpeza periodica de seguros expirados
	static void StartPeriodicCleanup()
	{
		int interval = PIXStoreConfig.CLEANUP_INTERVAL_MINUTES * 60 * 1000;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PeriodicCleanup, interval, true);
	}

	static void PeriodicCleanup()
	{
		if (!PIXStoreConfig.ENABLE_AUTO_CLEANUP) return;

		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		int totalLimpos = 0;

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity())
			{
				string steamId = player.GetIdentity().GetPlainId();
				int limpos = LimparSegurosExpirados(steamId);
				totalLimpos += limpos;
			}
		}

		if (totalLimpos > 0)
		{
			PIXStoreLogManager.LogSistema("LIMPEZA", totalLimpos.ToString() + " seguros expirados removidos");
		}
	}

	static int LimparSegurosExpirados(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
		int removidos = 0;

		for (int i = data.veiculos.Count() - 1; i >= 0; i--)
		{
			PIXStoreVeiculoData v = data.veiculos.Get(i);
			if (v.seguroAtivo && IsSeguroExpirado(v))
			{
				v.seguroAtivo = false;
				removidos++;
			}
		}

		if (removidos > 0)
		{
			PIXStorePixCoin.SavePlayerData(steamId, data);
		}

		return removidos;
	}

	static bool IsSeguroExpirado(PIXStoreVeiculoData veiculo)
	{
		if (!veiculo.dataExpiracao || veiculo.dataExpiracao == "")
			return false;

		CF_Date now = CF_Date.Now();
		CF_Date expiry = PIXStoreVIP.ParseDateString(veiculo.dataExpiracao);

		if (!expiry)
			return false;

		return now.Compare(expiry) > 0;
	}
}
