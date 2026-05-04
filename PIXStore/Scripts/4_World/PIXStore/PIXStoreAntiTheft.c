// PIXStore - Protocolo Anti-Roubo
// Ao recuperar veiculo, o antigo e destruido/explodido para evitar duplicacao
// Se houver jogador dentro, NAO faz nada
class PIXStoreAntiTheft
{
	// Processar veiculo antigo apos recuperacao
	static void ProcessarVeiculoAntigo(PIXStoreVeiculoData veiculoData)
	{
		if (!PIXStoreConfig.DESTRUIR_VEICULO_ANTIGO)
			return;

		// Localizar veiculo antigo pelo UID
		EntityAI veiculoAntigo = LocalizarVeiculoPorUID(veiculoData.veiculoUID, veiculoData.classname);

		if (!veiculoAntigo)
		{
			PIXStoreLogManager.LogSistema("ANTI_ROUBO", "Veiculo antigo nao encontrado: " + veiculoData.veiculoUID);
			return;
		}

		CarScript car = CarScript.Cast(veiculoAntigo);
		if (!car)
			return;

		// CRITICO: Verificar se tem jogador dentro - se sim, NAO fazer nada
		if (TemJogadorDentro(car))
		{
			PIXStoreLogManager.LogSistema("ANTI_ROUBO", "Veiculo tem jogador dentro, ignorando destruicao: " + veiculoData.veiculoUID);
			return;
		}

		// Aplicar destruicao
		if (PIXStoreConfig.USAR_EXPLOSAO)
		{
			ExplodirVeiculo(car);
		}
		else
		{
			DeletarVeiculo(car);
		}

		PIXStoreLogManager.LogSistema("ANTI_ROUBO", "Veiculo antigo destruido: " + veiculoData.displayName + " UID: " + veiculoData.veiculoUID);
	}

	// Verificar se algum jogador esta dentro do veiculo
	static bool TemJogadorDentro(CarScript car)
	{
		if (!car)
			return false;

		// Verificar todas as posicoes de assento
		for (int i = 0; i < car.CrewSize(); i++)
		{
			Human crew = car.CrewMember(i);
			if (crew)
			{
				// Tem alguem no veiculo
				return true;
			}
		}

		return false;
	}

	// Localizar veiculo no mundo pelo UID
	static EntityAI LocalizarVeiculoPorUID(string uid, string classname)
	{
		// Buscar entre todos os veiculos no servidor
		ref array<CarScript> veiculos = new array<CarScript>();

		// Iterar por todas as entidades do tipo correto
		ref array<Object> objects = new array<Object>();
		ref array<CargoBase> proxyCargos = new array<CargoBase>();

		// Buscar em area ampla (todo o mapa)
		GetGame().GetObjectsAtPosition(Vector(7500, 0, 7500), 15000, objects, proxyCargos);

		foreach (Object obj : objects)
		{
			CarScript car = CarScript.Cast(obj);
			if (car && car.GetType() == classname)
			{
				// Comparar UID
				int netId1, netId2;
				car.GetNetworkID(netId1, netId2);
				string currentUID = classname + "_" + netId1.ToString() + "_" + netId2.ToString();

				if (currentUID == uid)
					return car;
			}
		}

		return null;
	}

	// Deletar veiculo (metodo silencioso)
	static void DeletarVeiculo(CarScript car)
	{
		if (!car) return;

		// Remover todos os attachments primeiro
		for (int i = car.GetInventory().AttachmentCount() - 1; i >= 0; i--)
		{
			EntityAI attachment = car.GetInventory().GetAttachmentFromIndex(i);
			if (attachment)
			{
				GetGame().ObjectDelete(attachment);
			}
		}

		// Deletar o veiculo
		GetGame().ObjectDelete(car);
	}

	// Explodir veiculo (metodo visual)
	static void ExplodirVeiculo(CarScript car)
	{
		if (!car) return;

		vector pos = car.GetPosition();

		// Aplicar dano maximo para explodir
		car.SetHealth("Engine", "Health", 0);
		car.SetHealth("", "Health", 0);

		// Criar explosao visual
		Particle.PlayOnObject(ParticleList.EXPANSION_FIRE_MEDIUM, car, Vector(0, 0, 0));

		// Deletar apos breve delay para mostrar a explosao
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DeletarVeiculoDelayed, 3000, false, car);
	}

	// Deletar veiculo com delay (apos explosao)
	static void DeletarVeiculoDelayed(CarScript car)
	{
		if (car)
		{
			DeletarVeiculo(car);
		}
	}
}
