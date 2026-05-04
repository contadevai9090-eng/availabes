// PIXStore - MissionGameplay (Cliente)
// Gerencia input, menus e RPCs do lado cliente
modded class MissionGameplay
{
	ref PIXStoreMenu m_PIXStoreMenu;
	ref PIXStoreConfirmDialog m_PIXStoreConfirmDialog;

	override void OnInit()
	{
		super.OnInit();

		// Registrar RPCs do lado cliente
		GetRPCManager().AddRPC("PIXStore", "ReceivePlayerData", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "UpdateMapMarkers", this, SingeplayerExecutionType.Client);
		GetRPCManager().AddRPC("PIXStore", "ReceiveStatusMessage", this, SingeplayerExecutionType.Client);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);

		if (GetGame().IsClient())
		{
			Input input = GetGame().GetInput();

			// Detectar input para abrir menu PIXStore (tecla I)
			if (input.LocalPress("UAOpenPIXStoreMenu", false))
			{
				CloseAllMenusAndOpenPIXStore();
			}

			// Detectar ESC para fechar menus PIXStore
			if (GetUApi().GetInputByID(UAUIBack).LocalPress())
			{
				UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();

				if (currentMenu)
				{
					PIXStoreMenu pixMenu;
					if (Class.CastTo(pixMenu, currentMenu))
					{
						pixMenu.Close();
						m_PIXStoreMenu = null;
						return;
					}

					PIXStoreConfirmDialog confirmDialog;
					if (Class.CastTo(confirmDialog, currentMenu))
					{
						confirmDialog.Close();
						m_PIXStoreConfirmDialog = null;
						return;
					}
				}

				// Verificar menus orfaos
				if (m_PIXStoreMenu && m_PIXStoreMenu.layoutRoot)
				{
					m_PIXStoreMenu.Close();
					m_PIXStoreMenu = null;
				}
				else if (m_PIXStoreConfirmDialog && m_PIXStoreConfirmDialog.layoutRoot)
				{
					m_PIXStoreConfirmDialog.Close();
					m_PIXStoreConfirmDialog = null;
				}
			}
		}
	}

	void CloseAllMenusAndOpenPIXStore()
	{
		UIManager uiManager = GetUIManager();

		// Fechar qualquer menu scriptado ativo
		UIScriptedMenu currentMenu = uiManager.GetMenu();
		if (currentMenu)
		{
			currentMenu.Close();
		}

		// Fechar menus nativos do DayZ
		CloseNativeDayZMenus(uiManager);

		// Abrir menu PIXStore com delay
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(OpenPIXStoreMenuSafe, 100, false);
	}

	void CloseNativeDayZMenus(UIManager uiManager)
	{
		array<int> nativeMenus = {
			MENU_INVENTORY,
			MENU_CHAT_INPUT,
			MENU_MAP,
			MENU_BOOK,
			MENU_NOTE,
			MENU_INSPECT
		};

		foreach (int menuId : nativeMenus)
		{
			UIScriptedMenu nativeMenu = uiManager.FindMenu(menuId);
			if (nativeMenu)
			{
				nativeMenu.Close();
			}
		}
	}

	void OpenPIXStoreMenuSafe()
	{
		if (m_PIXStoreMenu)
			m_PIXStoreMenu.Close();

		m_PIXStoreMenu = new PIXStoreMenu();
		GetGame().GetUIManager().ShowScriptedMenu(m_PIXStoreMenu, null);
	}

	// RPC: Receber dados do jogador do servidor
	void ReceivePlayerData(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Client)
		{
			Param1<ref PIXStorePlayerData> data;
			if (!ctx.Read(data))
			{
				Print("[PIXStore] ReceivePlayerData - ERRO ao ler dados RPC");
				return;
			}

			// Atualizar menu se estiver aberto
			if (m_PIXStoreMenu)
			{
				m_PIXStoreMenu.PopulateWithData(data.param1);
			}
		}
	}

	// RPC: Receber dados de marcadores do mapa
	void UpdateMapMarkers(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Client)
		{
			Param1<string> data;
			if (!ctx.Read(data))
				return;

			if (m_PIXStoreMenu)
			{
				m_PIXStoreMenu.UpdateMapMarkers(data.param1);
			}
		}
	}

	// RPC: Receber mensagem de status
	void ReceiveStatusMessage(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if (type == CallType.Client)
		{
			Param1<string> data;
			if (!ctx.Read(data))
				return;

			if (m_PIXStoreMenu)
			{
				m_PIXStoreMenu.SetStatus(data.param1);
			}
		}
	}

	override void OnMissionFinish()
	{
		if (m_PIXStoreMenu)
		{
			m_PIXStoreMenu.Close();
			m_PIXStoreMenu = null;
		}

		super.OnMissionFinish();
	}
}
