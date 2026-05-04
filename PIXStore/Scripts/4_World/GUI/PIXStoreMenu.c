// PIXStore - Menu Principal
// Interface com abas: Ativar Seguro | Rastrear Veiculo | Recuperar Veiculo | Meu Painel
class PIXStoreMenu : UIScriptedMenu
{
	// Widgets de titulo e info
	protected TextWidget m_titleText;
	protected TextWidget m_saldoText;
	protected TextWidget m_vipText;
	protected TextWidget m_playerInfoText;
	protected TextWidget m_cooldownText;
	protected TextWidget m_statusText;

	// Botoes de aba
	protected ButtonWidget m_btnTabSeguro;
	protected ButtonWidget m_btnTabRastrear;
	protected ButtonWidget m_btnTabRecuperar;
	protected ButtonWidget m_btnTabInfo;

	// Paineis de aba
	protected Widget m_tabSeguro;
	protected Widget m_tabRastrear;
	protected Widget m_tabRecuperar;
	protected Widget m_tabInfo;

	// Tab Seguro
	protected TextWidget m_seguroVeiculoAtual;
	protected TextWidget m_seguroCusto;
	protected ButtonWidget m_btnAtivarSeguro;
	protected TextListboxWidget m_veiculosList;

	// Tab Rastrear
	protected TextListboxWidget m_rastrearList;
	protected TextWidget m_rastrearPosicao;
	protected TextWidget m_rastrearDistancia;
	protected ButtonWidget m_btnRastrear;
	protected ButtonWidget m_btnAtualizarPos;

	// Tab Recuperar
	protected TextListboxWidget m_recuperarList;
	protected TextWidget m_recuperarInfo;
	protected TextWidget m_recuperarCooldown;
	protected ButtonWidget m_btnRecuperar;
	protected ButtonWidget m_btnRemoverSeguro;

	// Tab Info
	protected TextWidget m_infoSaldo;
	protected TextWidget m_infoVIP;
	protected TextWidget m_infoVIPExpira;
	protected TextWidget m_infoVeiculos;
	protected TextWidget m_infoCooldown;
	protected TextListboxWidget m_infoLogsList;

	// Botoes gerais
	protected ButtonWidget m_btnRefresh;
	protected ButtonWidget m_btnClose;

	// Estado
	protected int m_currentTab = 0;
	protected int m_selectedVeiculoIndex = -1;
	protected ref array<ref PIXStoreVeiculoData> m_cachedVeiculos;
	protected ref PIXStoreConfirmDialog m_confirmDialog;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("PIXStore/GUI/layouts/PIXStoreMenu.layout");

		// Titulo
		m_titleText = TextWidget.Cast(layoutRoot.FindAnyWidget("TitleText"));
		m_saldoText = TextWidget.Cast(layoutRoot.FindAnyWidget("SaldoText"));
		m_vipText = TextWidget.Cast(layoutRoot.FindAnyWidget("VIPText"));
		m_playerInfoText = TextWidget.Cast(layoutRoot.FindAnyWidget("PlayerInfoText"));
		m_cooldownText = TextWidget.Cast(layoutRoot.FindAnyWidget("CooldownText"));
		m_statusText = TextWidget.Cast(layoutRoot.FindAnyWidget("StatusText"));

		// Botoes de aba
		m_btnTabSeguro = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_tab_seguro"));
		m_btnTabRastrear = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_tab_rastrear"));
		m_btnTabRecuperar = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_tab_recuperar"));
		m_btnTabInfo = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_tab_info"));

		// Paineis
		m_tabSeguro = layoutRoot.FindAnyWidget("TabSeguro");
		m_tabRastrear = layoutRoot.FindAnyWidget("TabRastrear");
		m_tabRecuperar = layoutRoot.FindAnyWidget("TabRecuperar");
		m_tabInfo = layoutRoot.FindAnyWidget("TabInfo");

		// Tab Seguro
		m_seguroVeiculoAtual = TextWidget.Cast(layoutRoot.FindAnyWidget("SeguroVeiculoAtual"));
		m_seguroCusto = TextWidget.Cast(layoutRoot.FindAnyWidget("SeguroCusto"));
		m_btnAtivarSeguro = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_ativar_seguro"));
		m_veiculosList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("veiculos_list"));

		// Tab Rastrear
		m_rastrearList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("rastrear_list"));
		m_rastrearPosicao = TextWidget.Cast(layoutRoot.FindAnyWidget("RastrearPosicao"));
		m_rastrearDistancia = TextWidget.Cast(layoutRoot.FindAnyWidget("RastrearDistancia"));
		m_btnRastrear = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_rastrear"));
		m_btnAtualizarPos = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_atualizar_pos"));

		// Tab Recuperar
		m_recuperarList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("recuperar_list"));
		m_recuperarInfo = TextWidget.Cast(layoutRoot.FindAnyWidget("RecuperarInfo"));
		m_recuperarCooldown = TextWidget.Cast(layoutRoot.FindAnyWidget("RecuperarCooldown"));
		m_btnRecuperar = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_recuperar"));
		m_btnRemoverSeguro = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_remover_seguro"));

		// Tab Info
		m_infoSaldo = TextWidget.Cast(layoutRoot.FindAnyWidget("InfoSaldo"));
		m_infoVIP = TextWidget.Cast(layoutRoot.FindAnyWidget("InfoVIP"));
		m_infoVIPExpira = TextWidget.Cast(layoutRoot.FindAnyWidget("InfoVIPExpira"));
		m_infoVeiculos = TextWidget.Cast(layoutRoot.FindAnyWidget("InfoVeiculos"));
		m_infoCooldown = TextWidget.Cast(layoutRoot.FindAnyWidget("InfoCooldown"));
		m_infoLogsList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("info_logs_list"));

		// Botoes gerais
		m_btnRefresh = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_refresh"));
		m_btnClose = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_close"));

		// Solicitar dados do servidor
		RequestDataFromServer();

		// Auto-refresh a cada 15 segundos
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AutoRefresh, 15000, true);

		// Mostrar aba padrao
		ShowTab(0);

		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
		GetGame().GetMission().GetHud().Show(false);

		// Atualizar veiculo atual (se estiver em um)
		AtualizarVeiculoAtual();
	}

	// Solicitar dados do servidor via RPC
	void RequestDataFromServer()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.GetIdentity()) return;

		string steamId = player.GetIdentity().GetPlainId();
		GetRPCManager().SendRPC("PIXStore", "RequestPlayerData", new Param1<string>(steamId), true, null);
	}

	// Receber dados do servidor
	void PopulateWithData(ref PIXStorePlayerData data)
	{
		if (!data) return;

		// Atualizar saldo
		if (m_saldoText)
			m_saldoText.SetText("Saldo: " + data.pixCoinSaldo.ToString() + " PixCoin");

		// Atualizar VIP
		if (data.isVIP)
		{
			if (m_vipText)
				m_vipText.SetText("[VIP]");
		}
		else
		{
			if (m_vipText)
				m_vipText.SetText("");
		}

		// Cachear veiculos
		m_cachedVeiculos = new array<ref PIXStoreVeiculoData>();
		foreach (PIXStoreVeiculoData v : data.veiculos)
		{
			if (v.seguroAtivo)
				m_cachedVeiculos.Insert(v);
		}

		// Popular listas
		PopularListaVeiculos();

		// Atualizar info do painel
		AtualizarPainelInfo(data);

		// Atualizar custo
		if (m_seguroCusto)
		{
			int custo = PIXStoreConfig.CUSTO_ATIVAR_SEGURO;
			m_seguroCusto.SetText("Custo: " + custo.ToString() + " PixCoin");
		}
	}

	// Popular listas de veiculos em todas as abas
	void PopularListaVeiculos()
	{
		// Limpar listas
		if (m_veiculosList) m_veiculosList.ClearItems();
		if (m_rastrearList) m_rastrearList.ClearItems();
		if (m_recuperarList) m_recuperarList.ClearItems();

		if (!m_cachedVeiculos || m_cachedVeiculos.Count() == 0)
		{
			if (m_veiculosList) m_veiculosList.AddItem("Nenhum veiculo segurado", null, 0);
			if (m_rastrearList) m_rastrearList.AddItem("Nenhum veiculo para rastrear", null, 0);
			if (m_recuperarList) m_recuperarList.AddItem("Nenhum veiculo para recuperar", null, 0);
			return;
		}

		for (int i = 0; i < m_cachedVeiculos.Count(); i++)
		{
			PIXStoreVeiculoData v = m_cachedVeiculos.Get(i);
			string displayText = string.Format("[%1] %2 - Registrado: %3", (i + 1).ToString(), v.displayName, v.dataRegistro);

			if (m_veiculosList) m_veiculosList.AddItem(displayText, null, 0);
			if (m_rastrearList) m_rastrearList.AddItem(v.displayName + " | Pos: " + v.posicao, null, 0);
			if (m_recuperarList) m_recuperarList.AddItem(v.displayName + " | Expira: " + v.dataExpiracao, null, 0);
		}
	}

	// Atualizar painel de informacoes
	void AtualizarPainelInfo(ref PIXStorePlayerData data)
	{
		if (m_infoSaldo)
			m_infoSaldo.SetText("Saldo PixCoin: " + data.pixCoinSaldo.ToString());

		if (m_infoVIP)
		{
			if (data.isVIP)
				m_infoVIP.SetText("Status VIP: ATIVO");
			else
				m_infoVIP.SetText("Status VIP: Inativo");
		}

		if (m_infoVIPExpira)
		{
			if (data.isVIP && data.vipDataExpiracao != "")
				m_infoVIPExpira.SetText("VIP Expira: " + data.vipDataExpiracao);
			else
				m_infoVIPExpira.SetText("");
		}

		// Contar veiculos ativos
		int veiculosAtivos = 0;
		if (m_cachedVeiculos)
			veiculosAtivos = m_cachedVeiculos.Count();

		bool isVIP = data.isVIP;
		int maxVeiculos = PIXStoreConfig.GetMaxVeiculos(isVIP);

		if (m_infoVeiculos)
			m_infoVeiculos.SetText("Veiculos Segurados: " + veiculosAtivos.ToString() + "/" + maxVeiculos.ToString());

		// Cooldown
		string cooldownStatus = "Disponivel";
		if (data.ultimaRecuperacao != "")
		{
			// Calcular localmente
			cooldownStatus = "Ultima recuperacao: " + data.ultimaRecuperacao;
		}

		if (m_infoCooldown)
			m_infoCooldown.SetText("Cooldown: " + cooldownStatus);

		if (m_cooldownText)
			m_cooldownText.SetText("Cooldown: " + cooldownStatus);
	}

	// Atualizar veiculo atual (se jogador esta em um)
	void AtualizarVeiculoAtual()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;

		// Verificar se esta em um veiculo
		HumanCommandVehicle cmdVehicle = player.GetCommand_Vehicle();
		if (cmdVehicle)
		{
			Transport transport = cmdVehicle.GetTransport();
			if (transport)
			{
				if (m_seguroVeiculoAtual)
					m_seguroVeiculoAtual.SetText("Veiculo atual: " + transport.GetDisplayName());
				if (m_btnAtivarSeguro)
					m_btnAtivarSeguro.Enable(true);
				return;
			}
		}

		if (m_seguroVeiculoAtual)
			m_seguroVeiculoAtual.SetText("Veiculo atual: Nenhum (entre em um veiculo)");
		if (m_btnAtivarSeguro)
			m_btnAtivarSeguro.Enable(false);
	}

	// Mostrar aba
	void ShowTab(int tabIndex)
	{
		m_currentTab = tabIndex;
		m_selectedVeiculoIndex = -1;

		if (m_tabSeguro) m_tabSeguro.Show(tabIndex == 0);
		if (m_tabRastrear) m_tabRastrear.Show(tabIndex == 1);
		if (m_tabRecuperar) m_tabRecuperar.Show(tabIndex == 2);
		if (m_tabInfo) m_tabInfo.Show(tabIndex == 3);

		// Atualizar dados ao trocar de aba
		if (tabIndex == 0)
			AtualizarVeiculoAtual();
	}

	// Auto-refresh
	void AutoRefresh()
	{
		if (layoutRoot)
		{
			RequestDataFromServer();
			AtualizarVeiculoAtual();
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		// Abas
		if (w == m_btnTabSeguro) { ShowTab(0); return true; }
		if (w == m_btnTabRastrear) { ShowTab(1); return true; }
		if (w == m_btnTabRecuperar) { ShowTab(2); return true; }
		if (w == m_btnTabInfo) { ShowTab(3); return true; }

		// Ativar Seguro
		if (w == m_btnAtivarSeguro)
		{
			OnAtivarSeguro();
			return true;
		}

		// Rastrear
		if (w == m_btnRastrear)
		{
			OnRastrear();
			return true;
		}

		if (w == m_btnAtualizarPos)
		{
			OnAtualizarPosicao();
			return true;
		}

		// Recuperar
		if (w == m_btnRecuperar)
		{
			OnRecuperar();
			return true;
		}

		// Remover seguro
		if (w == m_btnRemoverSeguro)
		{
			OnRemoverSeguro();
			return true;
		}

		// Geral
		if (w == m_btnRefresh)
		{
			RequestDataFromServer();
			AtualizarVeiculoAtual();
			SetStatus("Dados atualizados");
			return true;
		}

		if (w == m_btnClose)
		{
			Close();
			return true;
		}

		return false;
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
	{
		if (w == m_rastrearList || w == m_recuperarList || w == m_veiculosList)
		{
			m_selectedVeiculoIndex = row;
			AtualizarInfoVeiculoSelecionado();
			return true;
		}
		return false;
	}

	// Atualizar informacoes do veiculo selecionado
	void AtualizarInfoVeiculoSelecionado()
	{
		if (!m_cachedVeiculos || m_selectedVeiculoIndex < 0 || m_selectedVeiculoIndex >= m_cachedVeiculos.Count())
			return;

		PIXStoreVeiculoData v = m_cachedVeiculos.Get(m_selectedVeiculoIndex);

		if (m_rastrearPosicao)
			m_rastrearPosicao.SetText("Posicao: " + v.posicao);

		if (m_recuperarInfo)
			m_recuperarInfo.SetText("Veiculo: " + v.displayName + " | Tipo: " + v.classname);
	}

	// Acao: Ativar Seguro
	void OnAtivarSeguro()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;

		// Enviar RPC para servidor ativar seguro
		GetRPCManager().SendRPC("PIXStore", "AtivarSeguroRPC", null, true, null);
		SetStatus("Ativando seguro...");

		// Refresh apos 1 segundo
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RequestDataFromServer, 1000, false);
	}

	// Acao: Rastrear
	void OnRastrear()
	{
		if (m_selectedVeiculoIndex < 0)
		{
			SetStatus("Selecione um veiculo para rastrear");
			return;
		}

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;

		string steamId = player.GetIdentity().GetPlainId();

		// Solicitar rastreamento via RPC
		GetRPCManager().SendRPC("PIXStore", "RastrearVeiculoRPC", new Param1<int>(m_selectedVeiculoIndex), true, null);
		SetStatus("Rastreando veiculo...");
	}

	// Acao: Atualizar posicao
	void OnAtualizarPosicao()
	{
		RequestDataFromServer();
		SetStatus("Posicoes atualizadas");
	}

	// Acao: Recuperar
	void OnRecuperar()
	{
		if (m_selectedVeiculoIndex < 0)
		{
			SetStatus("Selecione um veiculo para recuperar");
			return;
		}

		if (!m_cachedVeiculos || m_selectedVeiculoIndex >= m_cachedVeiculos.Count())
			return;

		PIXStoreVeiculoData v = m_cachedVeiculos.Get(m_selectedVeiculoIndex);

		// Mostrar confirmacao
		ShowConfirmDialog("RECUPERAR VEICULO", "Deseja recuperar o veiculo?\nCusto: " + PIXStoreConfig.CUSTO_RECUPERAR_VEICULO.ToString() + " PixCoin", v.displayName, 0);
	}

	// Acao: Remover seguro
	void OnRemoverSeguro()
	{
		if (m_selectedVeiculoIndex < 0)
		{
			SetStatus("Selecione um veiculo");
			return;
		}

		if (!m_cachedVeiculos || m_selectedVeiculoIndex >= m_cachedVeiculos.Count())
			return;

		PIXStoreVeiculoData v = m_cachedVeiculos.Get(m_selectedVeiculoIndex);

		ShowConfirmDialog("REMOVER SEGURO", "Deseja remover o seguro deste veiculo?\nEsta acao nao pode ser desfeita!", v.displayName, 1);
	}

	// Mostrar dialogo de confirmacao
	void ShowConfirmDialog(string titulo, string mensagem, string itemName, int actionType)
	{
		if (m_confirmDialog)
		{
			m_confirmDialog.Close();
			m_confirmDialog = null;
		}

		m_confirmDialog = new PIXStoreConfirmDialog();
		m_confirmDialog.SetConfirmationData(titulo, mensagem, itemName, m_selectedVeiculoIndex, actionType, this);
		GetGame().GetUIManager().ShowScriptedMenu(m_confirmDialog, null);
	}

	// Executar acao confirmada
	void ExecuteConfirmedAction(int veiculoIndex, int actionType)
	{
		if (actionType == 0)
		{
			// Recuperar veiculo
			GetRPCManager().SendRPC("PIXStore", "RecuperarVeiculoRPC", new Param1<int>(veiculoIndex), true, null);
			SetStatus("Recuperando veiculo...");
		}
		else if (actionType == 1)
		{
			// Remover seguro
			GetRPCManager().SendRPC("PIXStore", "RemoverSeguroRPC", new Param1<int>(veiculoIndex), true, null);
			SetStatus("Removendo seguro...");
		}

		// Refresh apos 1.5 segundos
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RequestDataFromServer, 1500, false);
	}

	// Limpar referencia do dialogo
	void ClearConfirmDialogReference()
	{
		m_confirmDialog = null;
	}

	// Definir status na barra inferior
	void SetStatus(string msg)
	{
		if (m_statusText)
			m_statusText.SetText(msg);
	}

	// Receber dados de blip do servidor
	void UpdateMapMarkers(string markerData)
	{
		// Parsear dados de marcadores: "nome|x|z;nome|x|z;..."
		if (markerData == "") return;

		ref array<string> markers = new array<string>();
		markerData.Split(";", markers);

		// Atualizar info de rastreamento
		if (m_rastrearPosicao && markers.Count() > 0 && m_selectedVeiculoIndex >= 0 && m_selectedVeiculoIndex < markers.Count())
		{
			ref array<string> parts = new array<string>();
			markers.Get(m_selectedVeiculoIndex).Split("|", parts);
			if (parts.Count() >= 3)
			{
				m_rastrearPosicao.SetText("Posicao: X=" + parts.Get(1) + " Z=" + parts.Get(2));

				// Calcular distancia do jogador
				PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
				if (player)
				{
					vector playerPos = player.GetPosition();
					float vx = parts.Get(1).ToFloat();
					float vz = parts.Get(2).ToFloat();
					float dist = vector.Distance(playerPos, Vector(vx, 0, vz));

					if (m_rastrearDistancia)
						m_rastrearDistancia.SetText("Distancia: " + Math.Round(dist).ToString() + "m");
				}
			}
		}
	}

	override void OnHide()
	{
		super.OnHide();

		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().GetHud().Show(true);

		if (GetGame() && GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY))
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(AutoRefresh);
		}

		if (m_confirmDialog)
		{
			m_confirmDialog.Close();
			m_confirmDialog = null;
		}
	}
}
