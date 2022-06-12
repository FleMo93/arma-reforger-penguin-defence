class SCR_DefenceStateGUILayout : SCR_InfoDisplay
{
	private Widget defenceStateGUI;
	private TextWidget textWidgetWaveNumber
	private SCR_DefenceGameMode gameMode;
	private bool uiOutdated = false;
	
    override void OnStartDraw(IEntity owner)
    {
        super.OnStartDraw(owner);

        if (!textWidgetWaveNumber)
		{
			Widget widgetWaveNumber = m_wRoot.FindWidget("Scale.Overlay.WaveNumber");
			textWidgetWaveNumber = TextWidget.Cast(widgetWaveNumber);
			if(!textWidgetWaveNumber)
			{
				Print("Could not find textWidgetWaveNumber", LogLevel.ERROR);
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
			gameMode.GetOnWavePrepare().Insert(OnWavePrepare);
		}

		uiOutdated = true;
    }
	
	override void OnStopDraw(IEntity owner)
	{
		if(!gameMode)
			return;

		gameMode.GetOnWavePrepare().Remove(OnWavePrepare);
	}
	
	private void OnWavePrepare()
	{
		uiOutdated = true;
	}
	
	protected void UpdateInfo()
	{
		int waveNumber = gameMode.GetCurrentWave();
		textWidgetWaveNumber.SetText("Wave: " + waveNumber.ToString());

		uiOutdated = false;
	}
	
	override event void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		if(uiOutdated)
			UpdateInfo();
	}
}