class SCR_AreaRestrictionGUILayout : SCR_InfoDisplay
{
	private SCR_DefenceGameMode m_gameMode;
	private TextWidget m_twWarning1;
	private TextWidget m_twWarning2;
	private TextWidget m_twWarning3;
	private vector m_vAreaCenter = vector.Zero;
	private float m_fAreaRadius = -1;
	
	override void OnStartDraw(IEntity owner)
    {
		super.OnStartDraw(owner);
		if(!m_gameMode)
		{
			m_gameMode = SCR_DefenceGameMode.Cast(GetGame().GetGameMode());
			if(!m_gameMode)
			{
				Print("Could not get GameMode", LogLevel.ERROR);
				return;
			}
		}
	
		if(!m_twWarning1)
		{
			m_twWarning1 = TextWidget.Cast(m_wRoot.FindWidget("Scale.Overlay.Warning1"));
			if(!m_twWarning1)
			{
				Print("Could not get text widget Warning1", LogLevel.ERROR);
				return;
			}
			m_twWarning1.SetVisible(false);
		}
		
		if(!m_twWarning2)
		{
			m_twWarning2 = TextWidget.Cast(m_wRoot.FindWidget("Scale.Overlay.Warning2"));
			if(!m_twWarning2)
			{
				Print("Could not get text widget Warning2", LogLevel.ERROR);
				return;
			}
			m_twWarning2.SetVisible(false);
		}
		
		if(!m_twWarning3)
		{
			m_twWarning3 = TextWidget.Cast(m_wRoot.FindWidget("Scale.Overlay.Warning3"));
			if(!m_twWarning3)
			{
				Print("Could not get text widget Warning3", LogLevel.ERROR);
				return;
			}
			m_twWarning3.SetVisible(false);
		}
	}
	
	override event void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		if(!m_gameMode || m_gameMode.GetCurrentWave() == 0)
			return;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(owner);
		if(!playerController)
		{
			Print("Could not cast PlayerController", LogLevel.ERROR);
			return;
		}
		
		IEntity controlledEntity = playerController.GetControlledEntity();
		if(!controlledEntity)
			return;

		float distance = m_gameMode.GetGameAreaRadius() - vector.Distance(controlledEntity.GetOrigin(), m_gameMode.GetGameAreaCenter());
		if(distance < 2.5)
		{
			m_twWarning1.SetVisible(false);
			m_twWarning2.SetVisible(false);
			m_twWarning3.SetVisible(true);
		}
		else if (distance < 6)
		{
			m_twWarning1.SetVisible(false);
			m_twWarning2.SetVisible(true);
			m_twWarning3.SetVisible(false);
		}
		else if(distance < 10)
		{
			m_twWarning1.SetVisible(true);
			m_twWarning2.SetVisible(false);
			m_twWarning3.SetVisible(false);
		}
		else
		{
			m_twWarning1.SetVisible(false);
			m_twWarning2.SetVisible(false);
			m_twWarning3.SetVisible(false);
		}
	}
}