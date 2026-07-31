// PIXStore - Sistema de Configuracao
// Arquivo de configuracao salvo em $profile:PackFazupix\PIXStore/PIXStoreConfig.json
class PIXStoreConfig
{
	static string CONFIG_PATH = "$profile:PackFazupix\\PIXStore\\PIXStoreConfig.json";

	// Custos em PixCoin
	static int CUSTO_ATIVAR_SEGURO = 100;
	static int CUSTO_RECUPERAR_VEICULO = 50;

	// Cooldown de recuperacao (em minutos)
	static int COOLDOWN_RECUPERACAO_MINUTOS = 30;

	// Raio de spawn do veiculo recuperado (metros)
	static float RAIO_SPAWN_RECUPERACAO = 10.0;

	// Veiculos
	static int MAX_VEICULOS_POR_JOGADOR = 3;
	static bool SPAWN_COM_COMBUSTIVEL = true;
	static bool SPAWN_PRISTINE = true;

	// Anti-roubo
	static bool DESTRUIR_VEICULO_ANTIGO = true;
	static bool USAR_EXPLOSAO = false;

	// Integracao com mapa (BasicMap)
	static bool ATIVAR_RASTREAMENTO_MAPA = true;
	static int INTERVALO_ATUALIZACAO_BLIP_MS = 5000;

	// VIP
	static int VIP_DURACAO_DIAS = 30;
	static int VIP_MAX_VEICULOS_EXTRA = 2;
	static float VIP_DESCONTO_RECUPERACAO = 0.5;

	// Limpeza automatica
	static int CLEANUP_INTERVAL_MINUTES = 10;
	static bool ENABLE_AUTO_CLEANUP = true;

	// Seguranca
	static int ANTI_SPAM_COOLDOWN_SECONDS = 5;
	static float DISTANCIA_MINIMA_SPAWN = 5.0;

	static void LoadConfig()
	{
		if (!FileExist(CONFIG_PATH))
		{
			SaveConfig();
			return;
		}

		ref PIXStoreConfigData data;
		JsonFileLoader<PIXStoreConfigData>.JsonLoadFile(CONFIG_PATH, data);

		if (data)
		{
			CUSTO_ATIVAR_SEGURO = data.custoAtivarSeguro;
			CUSTO_RECUPERAR_VEICULO = data.custoRecuperarVeiculo;
			COOLDOWN_RECUPERACAO_MINUTOS = data.cooldownRecuperacaoMinutos;
			RAIO_SPAWN_RECUPERACAO = data.raioSpawnRecuperacao;
			MAX_VEICULOS_POR_JOGADOR = data.maxVeiculosPorJogador;
			SPAWN_COM_COMBUSTIVEL = data.spawnComCombustivel;
			SPAWN_PRISTINE = data.spawnPristine;
			DESTRUIR_VEICULO_ANTIGO = data.destruirVeiculoAntigo;
			USAR_EXPLOSAO = data.usarExplosao;
			ATIVAR_RASTREAMENTO_MAPA = data.ativarRastreamentoMapa;
			INTERVALO_ATUALIZACAO_BLIP_MS = data.intervaloAtualizacaoBlipMs;
			VIP_DURACAO_DIAS = data.vipDuracaoDias;
			VIP_MAX_VEICULOS_EXTRA = data.vipMaxVeiculosExtra;
			VIP_DESCONTO_RECUPERACAO = data.vipDescontoRecuperacao;
			CLEANUP_INTERVAL_MINUTES = data.cleanupIntervalMinutes;
			ENABLE_AUTO_CLEANUP = data.enableAutoCleanup;
			ANTI_SPAM_COOLDOWN_SECONDS = data.antiSpamCooldownSeconds;
			DISTANCIA_MINIMA_SPAWN = data.distanciaMinimaSpawn;
		}
	}

	static void SaveConfig()
	{
		// Criar diretorios necessarios
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");

		ref PIXStoreConfigData data = new PIXStoreConfigData();
		data.custoAtivarSeguro = CUSTO_ATIVAR_SEGURO;
		data.custoRecuperarVeiculo = CUSTO_RECUPERAR_VEICULO;
		data.cooldownRecuperacaoMinutos = COOLDOWN_RECUPERACAO_MINUTOS;
		data.raioSpawnRecuperacao = RAIO_SPAWN_RECUPERACAO;
		data.maxVeiculosPorJogador = MAX_VEICULOS_POR_JOGADOR;
		data.spawnComCombustivel = SPAWN_COM_COMBUSTIVEL;
		data.spawnPristine = SPAWN_PRISTINE;
		data.destruirVeiculoAntigo = DESTRUIR_VEICULO_ANTIGO;
		data.usarExplosao = USAR_EXPLOSAO;
		data.ativarRastreamentoMapa = ATIVAR_RASTREAMENTO_MAPA;
		data.intervaloAtualizacaoBlipMs = INTERVALO_ATUALIZACAO_BLIP_MS;
		data.vipDuracaoDias = VIP_DURACAO_DIAS;
		data.vipMaxVeiculosExtra = VIP_MAX_VEICULOS_EXTRA;
		data.vipDescontoRecuperacao = VIP_DESCONTO_RECUPERACAO;
		data.cleanupIntervalMinutes = CLEANUP_INTERVAL_MINUTES;
		data.enableAutoCleanup = ENABLE_AUTO_CLEANUP;
		data.antiSpamCooldownSeconds = ANTI_SPAM_COOLDOWN_SECONDS;
		data.distanciaMinimaSpawn = DISTANCIA_MINIMA_SPAWN;

		JsonFileLoader<PIXStoreConfigData>.JsonSaveFile(CONFIG_PATH, data);
	}

	static int GetMaxVeiculos(bool isVIP)
	{
		if (isVIP)
			return MAX_VEICULOS_POR_JOGADOR + VIP_MAX_VEICULOS_EXTRA;
		return MAX_VEICULOS_POR_JOGADOR;
	}

	static int GetCustoRecuperacao(bool isVIP)
	{
		if (isVIP)
			return Math.Round(CUSTO_RECUPERAR_VEICULO * VIP_DESCONTO_RECUPERACAO);
		return CUSTO_RECUPERAR_VEICULO;
	}
}

class PIXStoreConfigData
{
	int custoAtivarSeguro = 100;
	int custoRecuperarVeiculo = 50;
	int cooldownRecuperacaoMinutos = 30;
	float raioSpawnRecuperacao = 10.0;
	int maxVeiculosPorJogador = 3;
	bool spawnComCombustivel = true;
	bool spawnPristine = true;
	bool destruirVeiculoAntigo = true;
	bool usarExplosao = false;
	bool ativarRastreamentoMapa = true;
	int intervaloAtualizacaoBlipMs = 5000;
	int vipDuracaoDias = 30;
	int vipMaxVeiculosExtra = 2;
	float vipDescontoRecuperacao = 0.5;
	int cleanupIntervalMinutes = 10;
	bool enableAutoCleanup = true;
	int antiSpamCooldownSeconds = 5;
	float distanciaMinimaSpawn = 5.0;

	void PIXStoreConfigData() {}
}
