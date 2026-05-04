// PIXStore - Dialogo de Confirmacao
class PIXStoreConfirmDialog : UIScriptedMenu
{
	protected ButtonWidget m_btnYes;
	protected ButtonWidget m_btnNo;
	protected TextWidget m_dialogTitle;
	protected RichTextWidget m_dialogMessage;
	protected TextWidget m_itemName;

	// Dados para a confirmacao
	protected string m_confirmTitle = "";
	protected string m_confirmMessage = "";
	protected string m_confirmItemName = "";
	protected int m_confirmItemIndex = -1;
	protected int m_confirmActionType = -1;
	protected ref PIXStoreMenu m_parentMenu;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("PIXStore/GUI/layouts/PIXStoreConfirmDialog.layout");

		m_btnYes = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_confirm_yes"));
		m_btnNo = ButtonWidget.Cast(layoutRoot.FindAnyWidget("btn_confirm_no"));
		m_dialogTitle = TextWidget.Cast(layoutRoot.FindAnyWidget("ConfirmDialogTitle"));
		m_dialogMessage = RichTextWidget.Cast(layoutRoot.FindAnyWidget("ConfirmDialogMessage"));
		m_itemName = TextWidget.Cast(layoutRoot.FindAnyWidget("ConfirmDialogItemName"));

		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);

		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UpdateDialogTexts, 100, false);
	}

	// Configurar dados do dialogo
	void SetConfirmationData(string titulo, string mensagem, string itemName, int itemIndex, int actionType, ref PIXStoreMenu parentMenu)
	{
		m_confirmTitle = titulo;
		m_confirmMessage = mensagem;
		m_confirmItemName = itemName;
		m_confirmItemIndex = itemIndex;
		m_confirmActionType = actionType;
		m_parentMenu = parentMenu;

		UpdateDialogTexts();
	}

	void UpdateDialogTexts()
	{
		if (m_dialogTitle)
			m_dialogTitle.SetText(m_confirmTitle);

		if (m_dialogMessage)
			m_dialogMessage.SetText(m_confirmMessage);

		if (m_itemName)
			m_itemName.SetText("\"" + m_confirmItemName + "\"");
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_btnYes)
		{
			if (m_parentMenu)
			{
				m_parentMenu.ExecuteConfirmedAction(m_confirmItemIndex, m_confirmActionType);
			}
			CloseAndRestoreParent();
			return true;
		}

		if (w == m_btnNo)
		{
			CloseAndRestoreParent();
			return true;
		}

		return false;
	}

	void CloseAndRestoreParent()
	{
		if (m_parentMenu)
		{
			m_parentMenu.ClearConfirmDialogReference();
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RestoreParentFocus, 50, false);
		}
		Close();
	}

	void RestoreParentFocus()
	{
		if (m_parentMenu && m_parentMenu.layoutRoot)
		{
			GetGame().GetInput().ChangeGameFocus(1);
			GetGame().GetUIManager().ShowUICursor(true);
			GetGame().GetMission().GetHud().Show(false);
		}
	}

	override bool OnKeyPress(Widget w, int x, int y, int key)
	{
		super.OnKeyPress(w, x, y, key);

		if (key == KeyCode.KC_RETURN || key == KeyCode.KC_NUMPADENTER)
		{
			if (m_parentMenu)
			{
				m_parentMenu.ExecuteConfirmedAction(m_confirmItemIndex, m_confirmActionType);
			}
			CloseAndRestoreParent();
			return true;
		}

		return false;
	}

	override void OnHide()
	{
		super.OnHide();

		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
		GetGame().GetMission().GetHud().Show(false);

		GetGame().GetUIManager().Back();

		m_parentMenu = null;
	}
}
