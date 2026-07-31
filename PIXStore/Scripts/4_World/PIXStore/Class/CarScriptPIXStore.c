// PIXStore - Extensao do CarScript para veiculos segurados
modded class CarScript
{
	protected bool m_PIXStoreInsured = false;
	protected string m_PIXStoreOwnerSteamId = "";

	void CarScript()
	{
		RegisterNetSyncVariableBool("m_PIXStoreInsured");
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_PIXStoreInsured);
		ctx.Write(m_PIXStoreOwnerSteamId);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;

		m_PIXStoreInsured = false;
		ctx.Read(m_PIXStoreInsured);

		m_PIXStoreOwnerSteamId = "";
		ctx.Read(m_PIXStoreOwnerSteamId);

		return true;
	}

	// Marcar veiculo como segurado pelo PIXStore
	void PIXStoreSetInsured(bool insured, string ownerSteamId = "")
	{
		m_PIXStoreInsured = insured;
		m_PIXStoreOwnerSteamId = ownerSteamId;
		SetSynchDirty();
	}

	bool PIXStoreIsInsured()
	{
		return m_PIXStoreInsured;
	}

	string PIXStoreGetOwner()
	{
		return m_PIXStoreOwnerSteamId;
	}
}
