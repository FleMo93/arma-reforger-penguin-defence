[EntityEditorProps(category: "GameScripted/GameMode", color: "0 0 255 255")]
class SCR_DefenceGameModeClass: SCR_BaseGameModeClass
{
};

enum SCR_EWaveState
{
	PREPARE = 0,
	INPROGRESS = 1,
	END = 2
};

//------------------------------------------------------------------------------------------------
class SCR_DefenceGameMode : SCR_BaseGameMode
{	
	static const int ENDREASON_DEFENDERWON = 1;
	static const int ENDREASON_ATTACKERWON = 2;
	
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
	protected SCR_DefenceEnemySpawnerComponent defenceEnemySpawnerComponent;
	protected SCR_DefenceSpawnerComponent defenceSpawnerComponent;
	protected SCR_EWaveState waveState = SCR_EWaveState.PREPARE;
	
	[RplProp(condition: RplCondition.None)]
	protected int currentWave = 0;
	
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
	SCR_EWaveState GetWaveState()
	{
		return waveState;
	}
	
	int GetCurrentWave()
	{
		return currentWave;
	}
	
	override event void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		
		if(!Replication.IsServer())
			return;

		IEntity defencePoint = GetGame().FindEntity("DefencePoint");
		if(!defencePoint)
		{
			Print("DefencePoint entity not found", LogLevel.ERROR);
			return;
		}
		vector defencePointTransform[4];
		defencePoint.GetWorldTransform(defencePointTransform);
		defenceZoneCenter = defencePointTransform[3];
		
		defenceEnemySpawnerComponent = SCR_DefenceEnemySpawnerComponent.Cast(FindComponent(SCR_DefenceEnemySpawnerComponent));
		defenceEnemySpawnerComponent.GetOnAllGroupsKilled().Insert(OnWaveEnd);
		
		defenceSpawnerComponent = SCR_DefenceSpawnerComponent.Cast(FindComponent(SCR_DefenceSpawnerComponent));
		defenceSpawnerComponent.GetOnRespawnTicketsChanged().Insert(OnRespawnTicketsChanged);
	};
	
	override void OnGameStart()
	{
		super.OnGameStart();
		OnDefenceZoneChanged();
	}
	
	protected override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{
		super.OnPlayerKilled(playerId, player, killer);
		CheckIfDefendersLost();
	}

	protected void OnDefenceZoneChanged()
	{
		Event_OnDefenceZoneChanged.Invoke(defenceZoneCenter, gameAreaRadius, defenceRadius);
		OnWavePrepare();
	}

	protected void OnWavePrepare()
	{
		currentWave += 1;
		waveState = SCR_EWaveState.PREPARE;
		Event_OnWavePrepare.Invoke(currentWave);
		GetGame().GetCallqueue().CallLater(OnWaveStart, timeBetweenWaves);
	}

	protected void OnWaveStart()
	{
		waveState = SCR_EWaveState.INPROGRESS;
		Event_OnWaveStart.Invoke(currentWave);
	}
	
	protected void OnWaveEnd()
	{
		waveState = SCR_EWaveState.END;
		Event_OnWaveEnd.Invoke(currentWave);
		OnWavePrepare();
	}
	
	protected void OnRespawnTicketsChanged()
	{
		CheckIfDefendersLost();
	}
	
	private void CheckIfDefendersLost()
	{
		if(defenceSpawnerComponent.GetRespawnTickets() != 0)
			return;
		
		array<int> players = {};
		PlayerManager playerManager = GetGame().GetPlayerManager();
		playerManager.GetAllPlayers(players);
		
		foreach(int playerId : players)
		{
			PlayerController pc = playerManager.GetPlayerController(playerId);
			IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
			if(!controlledEntity)
				continue;

			SCR_CharacterControllerComponent characterControllerComponent = SCR_CharacterControllerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterControllerComponent));
			if(!characterControllerComponent)
				continue;

			if(!characterControllerComponent.IsDead())
				return;
		}

		FactionManager factionManager = GetGame().GetFactionManager();
		Faction hostileFaction = factionManager.GetFactionByKey("USSR");
		array<int> winnerFactionIds = {};
		winnerFactionIds.Insert(factionManager.GetFactionIndex(hostileFaction));
		SCR_GameModeEndData endGameData = SCR_GameModeEndData.Create(ENDREASON_ATTACKERWON, null, winnerFactionIds);
		EndGameMode(endGameData);
	}
	
	void OnEnemyEnteredTrigger()
	{
		
	}
	
	override bool CanPlayerRespawn(int playerID)
	{
		// Handler has prio in spawn logic
		if (m_pRespawnHandlerComponent && !m_pRespawnHandlerComponent.CanPlayerSpawn(playerID))
				return false;

		// Respawn timers
 		if (waveState == SCR_EWaveState.INPROGRESS && m_RespawnTimerComponent)
			return m_RespawnTimerComponent.GetCanPlayerSpawn(playerID);

		return true;
	}
}