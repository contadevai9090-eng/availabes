// PIXStore - Sistema de Recuperacao de Veiculos
// Spawna nova instancia do veiculo e aplica protocolo anti-roubo
class PIXStoreRecovery
{
	// Recuperar veiculo segurado
	static bool RecuperarVeiculo(PlayerBase player, int veiculoIndex)
	{
		if (!player) return false;

		string steamId = player.GetIdentity().GetPlainId();

		// Anti-spam
		if (!PIXStoreSystemManager.VerificarAntiSpam(steamId))
		{
			player.MessageStatus("[PIXStore] Aguarde antes de usar novamente");
			return false;
		}

		// Obter dados do jogador
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		// Obter veiculos ativos e mapear indices
		ref array<int> indicesAtivos = new array<int>();
		for (int idx = 0; idx < data.veiculos.Count(); idx++)
		{
			if (data.veiculos.Get(idx).seguroAtivo)
				indicesAtivos.Insert(idx);
		}

		if (veiculoIndex < 0 || veiculoIndex >= indicesAtivos.Count())
		{
			player.MessageStatus("[PIXStore] Veiculo invalido");
			return false;
		}

		int realIndex = indicesAtivos.Get(veiculoIndex);
		PIXStoreVeiculoData veiculo = data.veiculos.Get(realIndex);

		// Verificar se seguro esta ativo
		if (!veiculo.seguroAtivo)
		{
			player.MessageStatus("[PIXStore] Seguro nao esta ativo para este veiculo");
			return false;
		}

		// Verificar cooldown
		if (!VerificarCooldown(steamId, data))
		{
			string tempoRestante = GetCooldownRestante(steamId);
			player.MessageStatus("[PIXStore] Cooldown ativo! Aguarde: " + tempoRestante);
			PIXStoreLogManager.LogFalha(steamId, "RECUPERACAO", "Cooldown ativo: " + tempoRestante);
			return false;
		}

		// Verificar custo
		bool isVIP = PIXStoreVIP.IsVIP(steamId);
		int custo = PIXStoreConfig.GetCustoRecuperacao(isVIP);

		if (!PIXStorePixCoin.TemSaldo(steamId, custo))
		{
			player.MessageStatus("[PIXStore] Saldo insuficiente! Necessario: " + custo.ToString() + " PixCoin");
			PIXStoreLogManager.LogFalha(steamId, "RECUPERACAO", "Saldo insuficiente");
			return false;
		}

		// Debitar custo
		if (!PIXStorePixCoin.Debitar(steamId, custo, "Recuperacao - " + veiculo.displayName))
		{
			player.MessageStatus("[PIXStore] Erro ao debitar PixCoin");
			return false;
		}

		// PROTOCOLO ANTI-ROUBO: Lidar com veiculo antigo ANTES de spawnar o novo
		PIXStoreAntiTheft.ProcessarVeiculoAntigo(veiculo);

		// Spawnar nova instancia do veiculo
		bool spawnSuccess = SpawnNovoVeiculo(player, veiculo);

		if (spawnSuccess)
		{
			// Registrar cooldown
			CF_Date now = CF_Date.Now();
			data.ultimaRecuperacao = now.Format(CF_Date.DATETIME);
			PIXStorePixCoin.SavePlayerData(steamId, data);

			PIXStoreLogManager.LogRecuperacao(steamId, veiculo.displayName, player.GetPosition().ToString());
			player.MessageStatus("[PIXStore] Veiculo recuperado: " + veiculo.displayName + " | Custo: " + custo.ToString() + " PixCoin");
			return true;
		}
		else
		{
			// Reembolsar se falhou
			PIXStorePixCoin.Creditar(steamId, custo, "REEMBOLSO");
			player.MessageStatus("[PIXStore] Erro ao recuperar veiculo. PixCoin reembolsado.");
			PIXStoreLogManager.LogFalha(steamId, "RECUPERACAO", "Falha ao spawnar veiculo");
			return false;
		}
	}

	// Spawnar nova instancia do veiculo
	static bool SpawnNovoVeiculo(PlayerBase player, PIXStoreVeiculoData veiculo)
	{
		// Calcular posicao de spawn (raio de 10m do jogador)
		vector playerPos = player.GetPosition();
		vector spawnPos = GetPosicaoSpawnSegura(playerPos);

		// Verificar distancia minima
		if (vector.Distance(playerPos, spawnPos) < PIXStoreConfig.DISTANCIA_MINIMA_SPAWN)
		{
			spawnPos = playerPos + (player.GetDirection() * PIXStoreConfig.RAIO_SPAWN_RECUPERACAO);
		}

		// Criar veiculo
		EntityAI novoVeiculo = GetGame().CreateObject(veiculo.classname, spawnPos, false, true, true);

		if (!novoVeiculo)
			return false;

		CarScript car = CarScript.Cast(novoVeiculo);
		if (!car)
		{
			GetGame().ObjectDelete(novoVeiculo);
			return false;
		}

		// Configurar veiculo
		if (PIXStoreConfig.SPAWN_COM_COMBUSTIVEL)
		{
			car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
			car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
			car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
			car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
		}

		if (PIXStoreConfig.SPAWN_PRISTINE)
		{
			car.SetHealth("Engine", "Health", car.GetMaxHealth("Engine", ""));
			car.SetHealth("", "Health", car.GetMaxHealth("", ""));
		}

		// Recriar attachments
		if (veiculo.attachments && veiculo.attachments.Count() > 0)
		{
			foreach (string attachmentClass : veiculo.attachments)
			{
				EntityAI attachment = car.GetInventory().CreateAttachment(attachmentClass);
				if (attachment && PIXStoreConfig.SPAWN_PRISTINE)
				{
					attachment.SetHealth("", "", attachment.GetMaxHealth("", ""));
				}
			}
		}

		// Atualizar UID do veiculo no registro
		int netId1, netId2;
		novoVeiculo.GetNetworkID(netId1, netId2);
		veiculo.veiculoUID = veiculo.classname + "_" + netId1.ToString() + "_" + netId2.ToString();
		veiculo.posicao = spawnPos.ToString();

		return true;
	}

	// Calcular posicao de spawn segura no raio configurado
	static vector GetPosicaoSpawnSegura(vector centro)
	{
		float raio = PIXStoreConfig.RAIO_SPAWN_RECUPERACAO;

		// Tentar encontrar posicao segura em volta do jogador
		for (int tentativa = 0; tentativa < 8; tentativa++)
		{
			float angulo = tentativa * 45.0;
			float rad = angulo * Math.DEG2RAD;

			vector offset = Vector(Math.Cos(rad) * raio, 0, Math.Sin(rad) * raio);
			vector testPos = centro + offset;

			// Ajustar para o terreno
			testPos[1] = GetGame().SurfaceY(testPos[0], testPos[2]);

			// Posicao valida se esta acima do chao
			if (testPos[1] > 0)
				return testPos;
		}

		// Fallback: posicao direta a frente
		return centro + Vector(raio, 0, 0);
	}

	// Verificar cooldown de recuperacao
	static bool VerificarCooldown(string steamId, ref PIXStorePlayerData data)
	{
		if (!data.ultimaRecuperacao || data.ultimaRecuperacao == "")
			return true;

		CF_Date now = CF_Date.Now();
		CF_Date ultimaRec = PIXStoreVIP.ParseDateString(data.ultimaRecuperacao);

		if (!ultimaRec)
			return true;

		// Calcular diferenca
		int nowTimestamp = now.GetTimestamp();
		int ultimaTimestamp = ultimaRec.GetTimestamp();
		int diffSeconds = nowTimestamp - ultimaTimestamp;
		int cooldownSeconds = PIXStoreConfig.COOLDOWN_RECUPERACAO_MINUTOS * 60;

		return diffSeconds >= cooldownSeconds;
	}

	// Obter tempo restante de cooldown formatado
	static string GetCooldownRestante(string steamId)
	{
		ref PIXStorePlayerData data = PIXStorePixCoin.GetPlayerData(steamId);

		if (!data.ultimaRecuperacao || data.ultimaRecuperacao == "")
			return "Disponivel";

		CF_Date now = CF_Date.Now();
		CF_Date ultimaRec = PIXStoreVIP.ParseDateString(data.ultimaRecuperacao);

		if (!ultimaRec)
			return "Disponivel";

		int nowTimestamp = now.GetTimestamp();
		int ultimaTimestamp = ultimaRec.GetTimestamp();
		int diffSeconds = nowTimestamp - ultimaTimestamp;
		int cooldownSeconds = PIXStoreConfig.COOLDOWN_RECUPERACAO_MINUTOS * 60;

		int restanteSeconds = cooldownSeconds - diffSeconds;
		if (restanteSeconds <= 0)
			return "Disponivel";

		int minutos = restanteSeconds / 60;
		int segundos = restanteSeconds % 60;

		return string.Format("%1m %2s", minutos, segundos);
	}
}
