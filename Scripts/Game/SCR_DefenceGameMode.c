[EntityEditorProps(category: "GameScripted/GameMode", color: "0 0 255 255")]
class SCR_DefenceGameModeClass: SCR_BaseGameModeClass
{
};

//------------------------------------------------------------------------------------------------
class SCR_DefenceGameMode : SCR_BaseGameMode
{	
	[Attribute("100", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float gameAreaRadius;
	[Attribute("10", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float defenceRadius;
	[Attribute("60000", uiwidget: UIWidgets.EditBox, category: "Waves", desc: "")]
	protected float timeBetweenWaves;

	protected ref ScriptInvoker Event_OnDefenceZoneChanged = new ScriptInvoker();
	protected ref ScriptInvoker Event_OnWavePrepare = new ScriptInvoker();
	protected ref ScriptInvoker Event_OnWaveStart = new ScriptInvoker();
	protected ref ScriptInvoker Event_OnWaveEnd = new ScriptInvoker();
	
	protected vector defenceZoneCenter;
	
	ScriptInvoker GetOnDefenceZoneChanged()
	{
		return Event_OnDefenceZoneChanged;
	}
	ScriptInvoker GetOnWavePrepare()
	{
		return Event_OnWavePrepare;
	}
	ScriptInvoker GetOnWaveStart()
	{
		return Event_OnWaveStart;
	}
	ScriptInvoker GetOnWaveEnd()
	{
		return Event_OnWaveEnd;
	}
	
	override event void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		IEntity defencePoint = GetGame().FindEntity("DefencePoint");
		if(!defencePoint)
		{
			Print("DefencePoint entity not found", LogLevel.ERROR);
			return;
		}
		vector defencePointTransform[4];
		defencePoint.GetWorldTransform(defencePointTransform);
		defenceZoneCenter = defencePointTransform[3];
	};
	
	override void OnGameStart()
	{
		super.OnGameStart();
		OnDefenceZoneChanged();
	}

	protected void OnDefenceZoneChanged()
	{
		Event_OnDefenceZoneChanged.Invoke(defenceZoneCenter, gameAreaRadius, defenceRadius);
		OnWavePrepare();
		GetGame().GetCallqueue().CallLater(OnWaveStart, timeBetweenWaves);
	}

	protected void OnWavePrepare()
	{
		Event_OnWavePrepare.Invoke();
	}

	protected void OnWaveStart()
	{
		Event_OnWaveStart.Invoke();
	}
	
	void OnEnemyEnteredTrigger()
	{
		
	}
}