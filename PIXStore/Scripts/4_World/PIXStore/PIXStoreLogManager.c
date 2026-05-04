// PIXStore - Sistema de Logs
// Registra todas as atividades em PIXStoreLogs.log
class PIXStoreLogManager
{
	static string LOG_PATH = "$profile:PackFazupix\\PIXStore\\PIXStoreLogs.log";

	static void Init()
	{
		MakeDirectory("$profile:PackFazupix");
		MakeDirectory("$profile:PackFazupix\\PIXStore");
	}

	// Log generico de atividade
	static void LogActivity(string steamId, string acao, string detalhes = "")
	{
		string timestamp = GetTimestampString();
		string entry = string.Format("[%1] [%2] [%3] %4", timestamp, steamId, acao, detalhes);
		WriteLog(entry);
	}

	// Log de compra de seguro
	static void LogCompraSeguro(string steamId, string veiculo, int custo)
	{
		LogActivity(steamId, "COMPRA_SEGURO", "Veiculo: " + veiculo + " | Custo: " + custo.ToString());
	}

	// Log de ativacao de seguro
	static void LogAtivacaoSeguro(string steamId, string veiculo, string uid)
	{
		LogActivity(steamId, "ATIVACAO_SEGURO", "Veiculo: " + veiculo + " | UID: " + uid);
	}

	// Log de recuperacao
	static void LogRecuperacao(string steamId, string veiculo, string posicao)
	{
		LogActivity(steamId, "RECUPERACAO", "Veiculo: " + veiculo + " | Pos: " + posicao);
	}

	// Log de falha
	static void LogFalha(string steamId, string acao, string motivo)
	{
		LogActivity(steamId, "FALHA_" + acao, motivo);
	}

	// Log de abuso detectado
	static void LogAbuso(string steamId, string tipo, string detalhes)
	{
		LogActivity(steamId, "ABUSO_" + tipo, detalhes);
	}

	// Log de admin
	static void LogAdmin(string adminId, string acao, string targetId, string detalhes = "")
	{
		string timestamp = GetTimestampString();
		string entry = string.Format("[%1] [ADMIN:%2] [%3] Target: %4 | %5", timestamp, adminId, acao, targetId, detalhes);
		WriteLog(entry);
	}

	// Log de sistema
	static void LogSistema(string acao, string detalhes = "")
	{
		string timestamp = GetTimestampString();
		string entry = string.Format("[%1] [SISTEMA] [%2] %3", timestamp, acao, detalhes);
		WriteLog(entry);
	}

	// Log de VIP
	static void LogVIP(string steamId, string acao, string detalhes = "")
	{
		LogActivity(steamId, "VIP_" + acao, detalhes);
	}

	// Escrever no arquivo de log
	static void WriteLog(string entry)
	{
		FileHandle file = OpenFile(LOG_PATH, FileMode.APPEND);
		if (file)
		{
			FPrintln(file, entry);
			CloseFile(file);
		}
		Print("[PIXStore] " + entry);
	}

	// Obter timestamp formatado
	static string GetTimestampString()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		return string.Format("%1-%2-%3 %4:%5:%6",
			year.ToString(),
			FormatNum(month),
			FormatNum(day),
			FormatNum(hour),
			FormatNum(minute),
			FormatNum(second));
	}

	static string FormatNum(int num)
	{
		if (num < 10)
			return "0" + num.ToString();
		return num.ToString();
	}

	// Obter logs recentes
	static ref array<string> GetRecentLogs(int maxLines = 50)
	{
		ref array<string> logs = new array<string>();

		if (!FileExist(LOG_PATH))
			return logs;

		FileHandle file = OpenFile(LOG_PATH, FileMode.READ);
		if (file)
		{
			string line;
			ref array<string> allLines = new array<string>();

			while (FGets(file, line) >= 0)
			{
				allLines.Insert(line);
			}
			CloseFile(file);

			int startIndex = 0;
			if (allLines.Count() > maxLines)
				startIndex = allLines.Count() - maxLines;

			for (int i = startIndex; i < allLines.Count(); i++)
			{
				logs.Insert(allLines.Get(i));
			}
		}

		return logs;
	}
}
