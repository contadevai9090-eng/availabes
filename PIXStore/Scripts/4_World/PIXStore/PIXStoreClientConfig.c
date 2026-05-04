// PIXStore - Configuracoes sincronizadas do servidor para o cliente
class PIXStoreClientConfig
{
	static int CUSTO_ATIVAR_SEGURO = 100;
	static int CUSTO_RECUPERAR_VEICULO = 50;
	static int COOLDOWN_RECUPERACAO_MINUTOS = 30;
	static int MAX_VEICULOS_POR_JOGADOR = 3;
	static bool ATIVAR_RASTREAMENTO_MAPA = true;
	static int VIP_MAX_VEICULOS_EXTRA = 2;

	static void UpdateFromServer(int custoSeguro, int custoRecuperar, int cooldown, int maxVeiculos, bool rastreamento, int vipExtra)
	{
		CUSTO_ATIVAR_SEGURO = custoSeguro;
		CUSTO_RECUPERAR_VEICULO = custoRecuperar;
		COOLDOWN_RECUPERACAO_MINUTOS = cooldown;
		MAX_VEICULOS_POR_JOGADOR = maxVeiculos;
		ATIVAR_RASTREAMENTO_MAPA = rastreamento;
		VIP_MAX_VEICULOS_EXTRA = vipExtra;
	}
}
