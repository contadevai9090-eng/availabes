// PIXStore - Painel Admin (F12)
// Interface admin para gerenciar PixCoin, VIP e jogadores
class PIXStoreAdminMenu : UIScriptedMenu
{
	protected TextWidget m_titleText;
	protected TextWidget m_statusText;

	// Painel esquerdo - jogadores
	protected TextListboxWidget m_playersList;
	protected TextWidget m_selectedInfo;
	protected TextWidget m_selectedSaldo;
	protected TextWidget m_selectedVIP;
	protected ButtonWidget m_btnRefreshPlayers;

	// Painel direito - acoes
	protected EditBoxWidget m_steamIdInput;
	protected EditBoxWidget m_valorInput;
	protected ButtonWidget m_btnDarPixCoin;
	protected ButtonWidget m_btnRemoverPixCoin;
	protected ButtonWidget m_btnDarVIP;
	protected ButtonWidget m_btnRemoverVIP;
	protected ButtonWidget m_btnVerSaldo;
	protected ButtonWidget m_btnVerVIP;
	protected TextListboxWidget m_logList;
	protected ButtonWidget m_btnClose;

	// Estado
	protected ref array<string> m_playerSteamIds;
	protected int m_selectedPlayerIndex = -1;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("PIXStore/GUI/layouts/PIXStoreAdminMenu.layout");

		if (!layoutRoot)
		{
			Print("[PIXStore] ERRO: Admin layout nao encontrado!");
			return layoutRoot;
		}

		m_titleText = TextWidget.Cast(layoutRoot.FindAnyWidget("AdminTitleText"));
		m_statusText = TextWidget.Cast(layoutRoot.FindAnyWidget("AdminStatusText"));

		// Painel esquerdo
		m_playersList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("admin_players_list"));
		m_selectedInfo = TextWidget.Cast(layoutRoot.FindAnyWidget("AdminSelectedInfo"));
		m_selectedSaldo = TextWidget.Cast(layoutRoot.FindAnyWidget("AdminSelectedSaldo"));
		m_selectedVIP = TextWidget.Cast(layoutRoot.FindAnyWidget("AdminSelectedVIP"));
		m_btnRefreshPlayers = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_refresh_players"));

		// Painel direito
		m_steamIdInput = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("admin_steamid_input"));
		m_valorInput = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("admin_valor_input"));
		m_btnDarPixCoin = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_dar_pixcoin"));
		m_btnRemoverPixCoin = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_remover_pixcoin"));
		m_btnDarVIP = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_dar_vip"));
		m_btnRemoverVIP = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_remover_vip"));
		m_btnVerSaldo = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_ver_saldo"));
		m_btnVerVIP = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_ver_vip"));
		m_logList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("admin_log_list"));
		m_btnClose = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_admin_close"));

		m_playerSteamIds = new array<string>();

		// Solicitar lista de jogadores do servidor
		RequestAdminData();

		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
		GetGame().GetMission().GetHud().Show(false);
	}

	void RequestAdminData()
	{
		GetRPCManager().SendRPC("PIXStore", "AdminRequestPlayerList", null, true, null);
	}

	// Receber lista de jogadores do servidor
	void PopulatePlayerList(string playersData)
	{
		if (!m_playersList) return;

		m_playersList.ClearItems();
		m_playerSteamIds.Clear();

		if (playersData == "" || playersData.Length() == 0)
		{
			m_playersList.AddItem("Nenhum jogador online", null, 0);
			return;
		}

		ref array<string> players = new array<string>();
		playersData.Split(";", players);

		for (int i = 0; i < players.Count(); i++)
		{
			ref array<string> info = new array<string>();
			players.Get(i).Split("|", info);

			if (info.Count() >= 2)
			{
				string nome = info.Get(0);
				string steamId = info.Get(1);
				string saldo = "0";
				string vip = "Nao";

				if (info.Count() >= 3)
					saldo = info.Get(2);
				if (info.Count() >= 4)
					vip = info.Get(3);

				m_playersList.AddItem(nome + " | " + steamId + " | $" + saldo + " | VIP:" + vip, null, 0);
				m_playerSteamIds.Insert(steamId);
			}
		}

		SetStatus("Lista atualizada: " + m_playerSteamIds.Count().ToString() + " jogadores");
	}

	// Receber resposta de acao admin
	void ReceiveAdminResponse(string response)
	{
		SetStatus(response);
		AddLog(response);

		// Atualizar apos acao
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RequestAdminData, 500, false);
	}

	void AddLog(string msg)
	{
		if (m_logList)
			m_logList.AddItem(msg, null, 0);
	}

	void SetStatus(string msg)
	{
		if (m_statusText)
			m_statusText.SetText(msg);
	}

	string GetTargetSteamId()
	{
		// Priorizar input manual
		if (m_steamIdInput)
		{
			string inputText = m_steamIdInput.GetText();
			if (inputText.Length() > 0)
				return inputText;
		}

		// Usar jogador selecionado na lista
		if (m_selectedPlayerIndex >= 0 && m_selectedPlayerIndex < m_playerSteamIds.Count())
			return m_playerSteamIds.Get(m_selectedPlayerIndex);

		return "";
	}

	int GetValor()
	{
		if (m_valorInput)
		{
			string text = m_valorInput.GetText();
			if (text.Length() > 0)
				return text.ToInt();
		}
		return 0;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_btnClose)
		{
			Close();
			return true;
		}

		if (w == m_btnRefreshPlayers)
		{
			RequestAdminData();
			return true;
		}

		if (w == m_btnDarPixCoin)
		{
			string steamId = GetTargetSteamId();
			int valor = GetValor();
			if (steamId.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }
			if (valor <= 0) { SetStatus("Digite um valor positivo"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminDarPixCoin", new Param2<string, int>(steamId, valor), true, null);
			SetStatus("Creditando " + valor.ToString() + " PixCoin...");
			return true;
		}

		if (w == m_btnRemoverPixCoin)
		{
			string steamId2 = GetTargetSteamId();
			int valor2 = GetValor();
			if (steamId2.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }
			if (valor2 <= 0) { SetStatus("Digite um valor positivo"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminRemoverPixCoin", new Param2<string, int>(steamId2, valor2), true, null);
			SetStatus("Removendo " + valor2.ToString() + " PixCoin...");
			return true;
		}

		if (w == m_btnDarVIP)
		{
			string steamId3 = GetTargetSteamId();
			int dias = GetValor();
			if (steamId3.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }
			if (dias <= 0) { SetStatus("Digite os dias de VIP"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminDarVIP", new Param2<string, int>(steamId3, dias), true, null);
			SetStatus("Ativando VIP " + dias.ToString() + " dias...");
			return true;
		}

		if (w == m_btnRemoverVIP)
		{
			string steamId4 = GetTargetSteamId();
			if (steamId4.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminRemoverVIP", new Param1<string>(steamId4), true, null);
			SetStatus("Removendo VIP...");
			return true;
		}

		if (w == m_btnVerSaldo)
		{
			string steamId5 = GetTargetSteamId();
			if (steamId5.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminVerSaldo", new Param1<string>(steamId5), true, null);
			return true;
		}

		if (w == m_btnVerVIP)
		{
			string steamId6 = GetTargetSteamId();
			if (steamId6.Length() == 0) { SetStatus("Selecione jogador ou digite SteamID"); return true; }

			GetRPCManager().SendRPC("PIXStore", "AdminVerVIP", new Param1<string>(steamId6), true, null);
			return true;
		}

		return false;
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
	{
		if (w == m_playersList)
		{
			m_selectedPlayerIndex = row;

			if (m_selectedPlayerIndex >= 0 && m_selectedPlayerIndex < m_playerSteamIds.Count())
			{
				string steamId = m_playerSteamIds.Get(m_selectedPlayerIndex);

				if (m_steamIdInput)
					m_steamIdInput.SetText(steamId);

				if (m_selectedInfo)
					m_selectedInfo.SetText("Selecionado: " + steamId);

				// Solicitar info detalhada
				GetRPCManager().SendRPC("PIXStore", "AdminVerSaldo", new Param1<string>(steamId), true, null);
			}
			return true;
		}
		return false;
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().GetHud().Show(true);
	}
}
