//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Defence", description: "Handles the respawn of the .")]
class SCR_DefenceSpawnerComponentClass: SCR_RespawnHandlerComponentClass
{
};

//------------------------------------------------------------------------------------------------
/*
	Component responsible for handling automatic spawning logic.
*/
class SCR_DefenceSpawnerComponent : SCR_RespawnHandlerComponent
{
	
	[Attribute("-1", uiwidget: UIWidgets.EditComboBox, category: "Respawn", desc: "Faction index to spawn player(s) with. Only applied when greater or equal to 0.")]
	protected int m_iForcedFaction;
	
	[Attribute("", uiwidget: UIWidgets.EditComboBox, category: "Respawn Points", desc: "Trigger where players can spawn in.")]
	protected string respawnTriggerEntityName;

	[Attribute("20", uiwidget: UIWidgets.Slider, params: "0, 999, 1", category: "Respawn", desc: "Tickets for respawning.")]
	protected int respawnTickets;

	protected SCR_DefenceGameMode gameMode;
	protected BaseGameTriggerEntity defendPointTrigger;
	protected ref ScriptInvoker Event_OnRespawnTicketsChanged = new ScriptInvoker();
	protected SCR_BaseScoringSystemComponent scoringSystemComponent;
	
	/*!
		Batch of players that are supposed to spawn.
		Used to prevent modifying collection we're iterating through.
	*/
	private ref array<int> m_aSpawningBatch = {};
	
	int GetRespawnTickets()
	{
		return respawnTickets;
	}

	ScriptInvoker GetOnRespawnTicketsChanged()
	{
		return Event_OnRespawnTicketsChanged;
	}
	
	override void OnWorldPostProcess(World world)
	{
		IEntity respawnTriggerEntity = world.FindEntityByName(respawnTriggerEntityName);;
		defendPointTrigger = BaseGameTriggerEntity.Cast(respawnTriggerEntity);
		if (!defendPointTrigger)
		{
			Print("Respawn trigger entity not found", LogLevel.ERROR);
			return;
		}
		
		gameMode = SCR_DefenceGameMode.Cast(GetGameMode());
		if(!gameMode)
		{
			Print("Cant cast game mode", LogLevel.ERROR);
			return;
		}
		
		scoringSystemComponent = GetGameMode().GetScoringSystemComponent();
		if(!scoringSystemComponent)
		{
			Print("Cant get scoring system component.", LogLevel.ERROR);
			return;
		}
	}

	/*!
		When player is enqueued, randomize their loadout.
	*/
	override void OnPlayerEnqueued(int playerId)
	{
		super.OnPlayerEnqueued(playerId);
		if (m_iForcedFaction < 0)
		{
			Print(string.Format("Faction is not set"), LogLevel.ERROR);
			return;
		}

		if (SCR_RespawnSystemComponent.GetInstance().CanSetFaction(playerId, m_iForcedFaction))
				SCR_RespawnSystemComponent.GetInstance().DoSetPlayerFaction(playerId, m_iForcedFaction);
		else
			Print(string.Format("Cannot set faction %1 to player %2! Is faction index valid?", m_iForcedFaction, playerId), LogLevel.ERROR);

		
		RandomizePlayerLoadout(playerId);
		RandomizePlayerSpawnPoint(playerId);
	}
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		super.OnPlayerSpawned(playerId, controlledEntity);

		if (!gameMode.IsMaster())
			return;

		RandomGenerator randomGenerator = new RandomGenerator();
		randomGenerator.SetSeed(Math.RandomInt(0,100));
		vector location = GetSpawnPointInTrigger(randomGenerator);
		vector entityWorldTransform[4];
		controlledEntity.GetWorldTransform(entityWorldTransform);
		entityWorldTransform[3] = location;
		BaseGameEntity gameEntity = BaseGameEntity.Cast(controlledEntity);
		if(!gameEntity)
			return;
		gameEntity.Teleport(entityWorldTransform);
		
		SCR_ScoreInfo scoreInfo = scoringSystemComponent.GetPlayerScoreInfo(playerId);
		bool wasAlreadyDead = scoreInfo.m_iDeaths > 0 || scoreInfo.m_iSuicides > 0;
		if(gameMode.GetWaveState() == SCR_EWaveState.INPROGRESS && wasAlreadyDead)
		{
			respawnTickets--;
			Event_OnRespawnTicketsChanged.Invoke(respawnTickets);
		}
	}
	
	protected vector GetSpawnPointInTrigger(RandomGenerator randomGenerator)
	{
		if (!defendPointTrigger)
			return Vector(0, 0, 0);

		vector triggerLocation[4];
		defendPointTrigger.GetWorldTransform(triggerLocation);
		vector spawnPoint = randomGenerator.GenerateRandomPointInRadius(0, defendPointTrigger.GetSphereRadius(), triggerLocation[3], true);
		float elevation = GetGame().GetWorld().GetSurfaceY(spawnPoint[0], spawnPoint[2]);
		spawnPoint[1] = elevation;
		return spawnPoint;
	}

	/*!
		Ticks every frame. Handles automatic player respawn.
	*/
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Authority only
		if (!gameMode.IsMaster())
			return;

		// Clear batch
		m_aSpawningBatch.Clear();

		// Find players eligible for respawn
		foreach (int playerId : m_sEnqueuedPlayers)
		{
			if (gameMode.CanPlayerRespawn(playerId))
				m_aSpawningBatch.Insert(playerId);
		}

		// Respawn eligible players
		PlayerManager playerManager = GetGame().GetPlayerManager();
		foreach (int playerId : m_aSpawningBatch)
		{
			PlayerController playerController = playerManager.GetPlayerController(playerId);
			
			if(playerController)
				playerController.RequestRespawn();
		}

		super.EOnFrame(owner, timeSlice);
	}

	#ifdef WORKBENCH
	//! Possibility to get variable value choices dynamically
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntity owner, IEntityComponentSource src)
	{
		if (varName == "m_iForcedFaction")
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager)
			{
				ref array<ref ParamEnum> factionEnums = new array<ref ParamEnum>();
				factionEnums.Insert(new ParamEnum("Disabled", "-1"));

				Faction faction;
				string name;
				int factionCount = factionManager.GetFactionsCount();
				for (int i = 0; i < factionCount; i++)
				{
					faction = factionManager.GetFactionByIndex(i);
					name = faction.GetFactionKey();
					factionEnums.Insert(new ParamEnum(name, i.ToString()));
				}

				return factionEnums;
			}
		}

		return super._WB_GetUserEnums(varName, owner, src);
	}
	#endif
};