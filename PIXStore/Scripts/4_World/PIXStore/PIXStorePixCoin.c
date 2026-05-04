// PIXStore - Sistema de PixCoin (Moeda Virtual)
// Gerencia saldo de PixCoin por jogador (steamid)
// Saldo salvo em $profile:PackFazupix\PIXStore/bancoodedados/<steamid>.json
class PIXStorePixCoin
{
	static string GetPlayerDataPath(string steamId)
	{
		string dir = "$profile:PackFazupix\\PIXStore\\bancoodedados\\";
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");
		MakeDirectory("$profile:PackFazupix\\PIXStore\\bancoodedados");
		return dir + steamId + ".json";
	}

	// Obter dados completos do jogador
	static ref PIXStorePlayerData GetPlayerData(string steamId)
	{
		string path = GetPlayerDataPath(steamId);
		ref PIXStorePlayerData data;

		if (FileExist(path))
		{
			JsonFileLoader<PIXStorePlayerData>.JsonLoadFile(path, data);
		}

		if (!data)
		{
			data = new PIXStorePlayerData();
			data.steamId = steamId;
		}

		return data;
	}

	// Salvar dados do jogador
	static void SavePlayerData(string steamId, ref PIXStorePlayerData data)
	{
		string path = GetPlayerDataPath(steamId);
		JsonFileLoader<PIXStorePlayerData>.JsonSaveFile(path, data);
	}

	// Obter saldo de PixCoin
	static int GetSaldo(string steamId)
	{
		ref PIXStorePlayerData data = GetPlayerData(steamId);
		return data.pixCoinSaldo;
	}

	// Verificar se tem saldo suficiente
	static bool TemSaldo(string steamId, int valor)
	{
		return GetSaldo(steamId) >= valor;
	}

	// Debitar PixCoin (retorna true se bem-sucedido)
	static bool Debitar(string steamId, int valor, string motivo = "")
	{
		ref PIXStorePlayerData data = GetPlayerData(steamId);

		if (data.pixCoinSaldo < valor)
			return false;

		data.pixCoinSaldo = data.pixCoinSaldo - valor;
		SavePlayerData(steamId, data);

		PIXStoreLogManager.LogActivity(steamId, "DEBITO_PIXCOIN", "Valor: " + valor.ToString() + " | Motivo: " + motivo + " | Saldo: " + data.pixCoinSaldo.ToString());
		return true;
	}

	// Creditar PixCoin (admin)
	static void Creditar(string steamId, int valor, string adminId = "SYSTEM")
	{
		ref PIXStorePlayerData data = GetPlayerData(steamId);
		data.pixCoinSaldo = data.pixCoinSaldo + valor;
		SavePlayerData(steamId, data);

		PIXStoreLogManager.LogActivity(steamId, "CREDITO_PIXCOIN", "Valor: " + valor.ToString() + " | Admin: " + adminId + " | Saldo: " + data.pixCoinSaldo.ToString());
	}

	// Definir saldo diretamente (admin)
	static void SetSaldo(string steamId, int valor, string adminId = "SYSTEM")
	{
		ref PIXStorePlayerData data = GetPlayerData(steamId);
		int saldoAnterior = data.pixCoinSaldo;
		data.pixCoinSaldo = valor;
		SavePlayerData(steamId, data);

		PIXStoreLogManager.LogActivity(steamId, "SET_PIXCOIN", "Anterior: " + saldoAnterior.ToString() + " | Novo: " + valor.ToString() + " | Admin: " + adminId);
	}
}

// Dados de um veiculo segurado
class PIXStoreVeiculoData
{
	string classname;
	string displayName;
	string veiculoUID;
	string posicao;
	string dataRegistro;
	string dataExpiracao;
	ref array<string> attachments;
	bool seguroAtivo;

	void PIXStoreVeiculoData()
	{
		attachments = new array<string>();
		seguroAtivo = true;
	}
}

// Dados completos do jogador
class PIXStorePlayerData
{
	string steamId;
	int pixCoinSaldo;
	ref array<ref PIXStoreVeiculoData> veiculos;
	// VIP
	bool isVIP;
	string vipDataExpiracao;
	// Cooldowns
	string ultimaRecuperacao;
	// Anti-spam
	int ultimoComandoTimestamp;

	void PIXStorePlayerData()
	{
		pixCoinSaldo = 0;
		veiculos = new array<ref PIXStoreVeiculoData>();
		isVIP = false;
		vipDataExpiracao = "";
		ultimaRecuperacao = "";
		ultimoComandoTimestamp = 0;
	}
}
