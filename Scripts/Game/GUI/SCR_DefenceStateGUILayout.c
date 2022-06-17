class SCR_DefenceStateGUILayout : SCR_InfoDisplay
{
	private TextWidget textWidgetWaveNumber;
	private TextWidget textWidgetRespawnTickets;
	private SCR_DefenceGameMode gameMode;
	private SCR_DefenceSpawnerComponent spawnerComponent;
	private bool uiOutdated = false;
	
    override void OnStartDraw(IEntity owner)
    {
        super.OnStartDraw(owner);

        if (!textWidgetWaveNumber)
		{
			Widget widget = m_wRoot.FindWidget("Scale.Overlay.WaveNumber");
			textWidgetWaveNumber = TextWidget.Cast(widget);
			if(!textWidgetWaveNumber)
			{
				Print("Could not find textWidgetWaveNumber", LogLevel.ERROR);
				return;
			}
		}
		
		if(!textWidgetRespawnTickets)
		{
			Widget widget = m_wRoot.FindWidget("Scale.Overlay.RespawnTickets");
			textWidgetRespawnTickets = TextWidget.Cast(widget);
			if(!textWidgetRespawnTickets)
			{
				Print("Could not find textWidgetRespawnTickets", LogLevel.ERROR);
				return;
			}
		}
		

		if(!gameMode)
		{
			gameMode = SCR_DefenceGameMode.Cast(GetGame().GetGameMode());
			if(!gameMode)
			{
				Print("Could not get GameMode", LogLevel.ERROR);
				return;
			}
			gameMode.GetOnWavePrepare().Insert(SetUIOutdated);
		}
		
		if(!spawnerComponent && gameMode)
		{
			spawnerComponent = SCR_DefenceSpawnerComponent.Cast(gameMode.FindComponent(SCR_DefenceSpawnerComponent));
			if(!spawnerComponent)
			{
				Print("Could not get RespawnManager", LogLevel.ERROR);
				return;
			}
			spawnerComponent.GetOnRespawnTicketsChanged().Insert(SetUIOutdated);
		}

		uiOutdated = true;
    }
	
	override void OnStopDraw(IEntity owner)
	{
		if(gameMode)
			gameMode.GetOnWavePrepare().Remove(SetUIOutdated);
		
		if(spawnerComponent)
			spawnerComponent.GetOnRespawnTicketsChanged().Remove(SetUIOutdated);
	}
	
	private void SetUIOutdated()
	{
		uiOutdated = true;
	}
	
	protected void UpdateInfo()
	{
		int waveNumber = gameMode.GetCurrentWave();
		textWidgetWaveNumber.SetText("Wave: " + waveNumber.ToString());
		
		int respawnTickets = spawnerComponent.GetRespawnTickets();
		textWidgetRespawnTickets.SetText("Tickets: " + respawnTickets.ToString());

		uiOutdated = false;
	}
	
	override event void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		if(uiOutdated)
			UpdateInfo();
	}
}