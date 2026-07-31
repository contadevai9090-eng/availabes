// PIXStore - Sistema VIP com validade automatica
// VIP tem duracao configuravel e expira automaticamente
class PIXStoreVIP
{
	// Verificar se jogador e VIP (com checagem de expiracao)
	static bool IsVIP(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		if (!data.isVIP)
			return false;

		// Verificar se VIP expirou
		if (IsVIPExpirado(data))
		{
			// Revogar VIP automaticamente
			data.isVIP = false;
			data.vipDataExpiracao = "";
			PIXStorePixCoin.SavePlayerData(steamId, data);
			PIXStoreLogManager.LogVIP(steamId, "EXPIRADO", "VIP expirou automaticamente");
			return false;
		}

		return true;
	}

	// Ativar VIP para jogador
	static bool AtivarVIP(string steamId, int dias, string adminId = "SYSTEM")
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		// Calcular data de expiracao
		CF_Date expiracao = CF_Date.Now();
		int currentTimestamp = expiracao.GetTimestamp();
		int expirationTimestamp = currentTimestamp + (dias * 86400);
		expiracao.EpochToDate(expirationTimestamp);

		data.isVIP = true;
		data.vipDataExpiracao = expiracao.Format(CF_Date.DATETIME);

		PIXStorePixCoin.SavePlayerData(steamId, data);
		PIXStoreLogManager.LogVIP(steamId, "ATIVADO", "Dias: " + dias.ToString() + " | Expira: " + data.vipDataExpiracao + " | Admin: " + adminId);

		return true;
	}

	// Estender VIP existente
	static bool EstenderVIP(string steamId, int dias, string adminId = "SYSTEM")
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		CF_Date baseDate;

		if (data.isVIP && data.vipDataExpiracao != "")
		{
			// Se ja e VIP, estender a partir da data atual de expiracao
			baseDate = ParseDateString(data.vipDataExpiracao);
			if (!baseDate)
				baseDate = CF_Date.Now();
		}
		else
		{
			baseDate = CF_Date.Now();
		}

		int baseTimestamp = baseDate.GetTimestamp();
		int newTimestamp = baseTimestamp + (dias * 86400);

		CF_Date novaExpiracao = CF_Date.Now();
		novaExpiracao.EpochToDate(newTimestamp);

		data.isVIP = true;
		data.vipDataExpiracao = novaExpiracao.Format(CF_Date.DATETIME);

		PIXStorePixCoin.SavePlayerData(steamId, data);
		PIXStoreLogManager.LogVIP(steamId, "ESTENDIDO", "Dias: " + dias.ToString() + " | Nova Expiracao: " + data.vipDataExpiracao + " | Admin: " + adminId);

		return true;
	}

	// Revogar VIP
	static void RevogarVIP(string steamId, string adminId = "SYSTEM")
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
		data.isVIP = false;
		data.vipDataExpiracao = "";
		PIXStorePixCoin.SavePlayerData(steamId, data);
		PIXStoreLogManager.LogVIP(steamId, "REVOGADO", "Admin: " + adminId);
	}

	// Obter tempo restante de VIP formatado
	static string GetTempoRestanteFormatado(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		if (!data.isVIP || data.vipDataExpiracao == "")
			return "Sem VIP";

		CF_Date now = CF_Date.Now();
		CF_Date expiry = ParseDateString(data.vipDataExpiracao);

		if (!expiry)
			return "Erro de data";

		if (now.Compare(expiry) > 0)
			return "Expirado";

		int hoursDiff, minutesDiff;
		now.CalculateDifference(expiry, hoursDiff, minutesDiff);

		int dias = hoursDiff / 24;
		int horas = hoursDiff % 24;

		if (dias > 0)
			return string.Format("%1d %2h %3m", dias, horas, minutesDiff);
		else if (horas > 0)
			return string.Format("%1h %2m", horas, minutesDiff);
		else
			return string.Format("%1m", minutesDiff);
	}

	// Obter data de expiracao
	static string GetDataExpiracao(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);
		if (!data.isVIP)
			return "N/A";
		return data.vipDataExpiracao;
	}

	// Verificar se VIP esta expirado
	static bool IsVIPExpirado(ref PIXStorePlayerData data)
	{
		if (!data.isVIP || data.vipDataExpiracao == "")
			return true;

		CF_Date now = CF_Date.Now();
		CF_Date expiry = ParseDateString(data.vipDataExpiracao);

		if (!expiry)
			return true;

		return now.Compare(expiry) > 0;
	}

	// Verificacao periodica de VIPs expirados (chamada pelo servidor)
	static void ChecarVIPsExpirados()
	{
		ref array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity())
			{
				string steamId = player.GetIdentity().GetPlainId();
				ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

				if (data.isVIP && IsVIPExpirado(data))
				{
					data.isVIP = false;
					data.vipDataExpiracao = "";
					PIXStorePixCoin.SavePlayerData(steamId, data);
					PIXStoreLogManager.LogVIP(steamId, "EXPIRADO_AUTO", "VIP removido automaticamente");

					player.MessageStatus("[PIXStore] Seu VIP expirou!");
				}
			}
		}
	}

	// Parser de data
	static CF_Date ParseDateString(string dateStr)
	{
		if (!dateStr || dateStr == "")
			return null;

		array<string> parts = new array<string>();
		dateStr.Split(" ", parts);

		if (parts.Count() != 2)
			return null;

		array<string> dateParts = new array<string>();
		parts[0].Split("-", dateParts);

		if (dateParts.Count() != 3)
			return null;

		int year = dateParts[0].ToInt();
		int month = dateParts[1].ToInt();
		int day = dateParts[2].ToInt();

		array<string> timeParts = new array<string>();
		parts[1].Split(":", timeParts);

		if (timeParts.Count() != 3)
			return null;

		int hour = timeParts[0].ToInt();
		int minute = timeParts[1].ToInt();
		int second = timeParts[2].ToInt();

		return CF_Date.CreateDateTime(year, month, day, hour, minute, second);
	}
}
