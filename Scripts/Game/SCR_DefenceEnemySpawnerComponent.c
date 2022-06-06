//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Defence", description: "Handles the loot.")]
class SCR_DefenceEnemySpawnerComponentClass: SCR_BaseGameModeComponentClass
{
};



//------------------------------------------------------------------------------------------------
/*
	Component responsible for handling automatic spawning logic.
*/
class SCR_DefenceEnemySpawnerComponent : SCR_BaseGameModeComponent
{
	private SCR_DefenceGameMode gameMode;
	private ArmaReforgerScripted game;
	private World _world;
	private PlayerManager playerManager;
	private vector _center;
	private float _gameAreaRadius;
	private float _defenceRadius;
	private ref RandomGenerator randomGenerator;
	
	
	[Attribute(params: "conf")]
	protected ResourceName waveTypesConfig;
	
	[Attribute(params: "conf")]
	protected ResourceName hostileCharactersConfig;
	
	[Attribute("25", uiwidget: UIWidgets.Slider)]
	protected float spawnDistanceOffset;
	
	[Attribute("USSR", uiwidget: UIWidgets.EditComboBox)]
	protected FactionKey hostileAffiliatedFaction;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "", "et")]
	protected ResourceName waypointResourceName;
	
	[Attribute("360", uiwidget: UIWidgets.EditComboBox)]
	protected int numberOfSpawnPoints;
	
	protected ref SCR_WaveTypes _waveTypesConfig;
	protected ref SCR_HostileCharacters _hostileCharacterConfig;
	protected AIWaypoint aiWaypoint;
	protected ref array<vector> spawnPoints = {};

	override event void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if(!Replication.IsServer())
			return;

		randomGenerator = new RandomGenerator();
		randomGenerator.SetSeed(Math.RandomInt(0, 100));
		
		if(waveTypesConfig)
		{
			Resource wavesResource = Resource.Load(waveTypesConfig);
			Managed wavesBaseContainer =  BaseContainerTools.CreateInstanceFromContainer(wavesResource.GetResource().ToBaseContainer());
			_waveTypesConfig = SCR_WaveTypes.Cast(wavesBaseContainer);
		}
		else
			Print("Missing wave types config. May lead to error on start.", LogLevel.WARNING);
		
		if(hostileCharactersConfig)
		{
			Resource hostileCharactersResource = Resource.Load(hostileCharactersConfig);
			Managed hostileCharactersBaseContainer =  BaseContainerTools.CreateInstanceFromContainer(hostileCharactersResource.GetResource().ToBaseContainer());
			_hostileCharacterConfig = SCR_HostileCharacters.Cast(hostileCharactersBaseContainer);
		}
		else
			Print("Missing hostile characters config. May lead to error on start.", LogLevel.WARNING);

		gameMode = SCR_DefenceGameMode.Cast(GetGameMode());
		game = GetGame();
		_world = world;
		playerManager = game.GetPlayerManager();
		if(!gameMode) Print("Could not parse game mode", LogLevel.ERROR);
		gameMode.GetOnDefenceZoneChanged().Insert(SetDefenceZone);
		gameMode.GetOnWaveStart().Insert(OnWaveStart);
		
		if(waypointResourceName)
		{
			Resource resource = Resource.Load(waypointResourceName);
			IEntity aiWaypointEntity = game.SpawnEntityPrefab(resource, world);
			aiWaypoint = AIWaypoint.Cast(aiWaypointEntity);
			aiWaypoint.SetCompletionType(EAIWaypointCompletionType.All);
		}
		else
			Print("Missing waypoint resource name. May lead to error on start.", LogLevel.WARNING);		
	}
	
	protected void SetDefenceZone(vector center, float gameArearadius, float defenceRadius)
	{
		_center = center;
		_gameAreaRadius = gameArearadius;
		_defenceRadius = defenceRadius;
		aiWaypoint.SetCompletionRadius(Math.Max(0.5, defenceRadius - 0.5));
		aiWaypoint.SetOrigin(_center);
		PrepareSpawnPoints();
		Print(string.Format("%1 spawn points found", spawnPoints.Count()), LogLevel.NORMAL);
	}
	
	protected SCR_ChimeraCharacter SpawnHostileCharacterAtRandomLocation(SCR_HostileCharacter hostileCharacter)
	{
		Resource hostileCharacterResource = hostileCharacter.GetResource();	
		EntitySpawnParams spawnParams = EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = {
			Vector(1, 0, 0),
			Vector(0, 1, 0),
			Vector(0, 0, 1),
			GetRandomSpawnPoint()
		};
		IEntity entity = game.SpawnEntityPrefab(hostileCharacter.GetResource(), _world, spawnParams);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if(!character)
		{
			Print(string.Format("Entity of type %1 is no ChimeraCharacter", hostileCharacter.GetResourceName()), LogLevel.ERROR);
			entity.GetParent().RemoveChild(entity);
			return null;
		}
		
		//SetCharacterAffiliatedFaction(character);
		//SetCharacterSkill(character);
		SetCharacterWaypoint(character);
		
		return character;
	}
	
	protected void SetCharacterWaypoint(SCR_ChimeraCharacter character)
	{
		AIControlComponent aiControlComponent = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		AIAgent aiAgent = aiControlComponent.GetControlAIAgent();

		array<AIWaypoint> activeWaypoints = {};
		aiAgent.GetWaypoints(activeWaypoints);
		foreach(AIWaypoint wpt : activeWaypoints)
			aiAgent.RemoveWaypoint(wpt);

		aiAgent.AddWaypoint(aiWaypoint);
		aiControlComponent.ActivateAI();
	}
	
	protected void SetCharacterAffiliatedFaction(SCR_ChimeraCharacter character)
	{
		FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		factionAffiliationComponent.SetAffiliatedFactionByKey(hostileAffiliatedFaction);
	}
	
	protected void SetCharacterSkill(SCR_ChimeraCharacter character)
	{
		SCR_AICombatComponent aiCombatComponent = SCR_AICombatComponent.Cast(character.FindComponent(SCR_AICombatComponent));
		aiCombatComponent.SetAISkill(GetHostileSkill());
	}
	
	protected void OnWaveStart()
	{
		array<ref SCR_WaveType> waveTypes = {};
		_waveTypesConfig.GetWaveTypes(waveTypes);
		int waveTypeIndex = Math.Floor(randomGenerator.RandFloat01() * (waveTypes.Count() - 1));
		SCR_WaveType waveType = waveTypes[waveTypeIndex];
		array<ref SCR_HostileCharacter> hostileCharacters = {};
		_hostileCharacterConfig.GetHostileCharactersByRelatedWaveName(hostileCharacters, waveType.GetName());
		
		for(int i = 0; i < GetEnemyAmount(); i++)
		{
			int hostileCharacterIndex = Math.Floor(randomGenerator.RandFloat01() * (hostileCharacters.Count() - 1));
			SCR_HostileCharacter hostileCharacter = hostileCharacters[hostileCharacterIndex];
			SpawnHostileCharacterAtRandomLocation(hostileCharacter);
		}
	}
	
	private bool PrepareSpawnPointsQueryCallback(IEntity entity)
	{
		return false;
	}
	
	protected void PrepareSpawnPoints()
	{
		float radianOffset = 6.28319 / numberOfSpawnPoints;
		for(int i = 0; i < numberOfSpawnPoints; i++)
		{
			float radians = radianOffset * i;
			vector direction = Vector(Math.Cos(radians), 0, Math.Sin(radians));
			vector spawnPoint = _center + direction * (_gameAreaRadius + spawnDistanceOffset);
			spawnPoint[1] = _world.GetSurfaceY(spawnPoint[0], spawnPoint[2]) + 1;
			if(_world.QueryEntitiesBySphere(spawnPoint, 1, PrepareSpawnPointsQueryCallback, null, EQueryEntitiesFlags.ALL))
			{
				spawnPoint[1] = spawnPoint[1] - 0.5;
				spawnPoints.Insert(spawnPoint);
			}
		}
	}
		
	protected vector GetRandomSpawnPoint()
	{
		return spawnPoints[Math.Floor(randomGenerator.RandFloat01() * (spawnPoints.Count() - 1))];
		float degree = randomGenerator.RandFloat01() * 360;
		float radians = degree * (Math.PI / 180);
		vector direction = Vector(Math.Cos(radians), 0, Math.Sin(radians));
		vector spawnPoint = _center + direction * (_gameAreaRadius + spawnDistanceOffset);
		spawnPoint[1] = _world.GetSurfaceY(spawnPoint[0], spawnPoint[2]);
		
		return spawnPoint;
	}
	
	protected int GetEnemyAmount()
	{
		return 10;//playerManager.GetPlayerCount();
	}
	
	protected EAISkill GetHostileSkill()
	{
		return EAISkill.EXPERT;
	}
	
	
	#ifdef WORKBENCH
	//! Possibility to get variable value choices dynamically
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntity owner, IEntityComponentSource src)
	{
		if (varName == "hostileAffiliatedFaction")
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
	
	static vector RandomPointInRadius(float maxDistance)
	{
		vector point = Vector(Math.RandomFloat(-1 , 1), 0, Math.RandomFloat(-1 , 1));
		point.Normalize();
		return point * (Math.RandomFloat01()  * maxDistance);
	}
	
	static ref array<IEntity> queriedEntities = {};
	
	static bool FindSafePositionQueryCallback(IEntity entity)
	{
		queriedEntities.Insert(entity);
		return false;
	}
	
	static vector FindSafePosition(World world, vector center, float maxDistance, float objDistance)
	{
		vector location;
		bool locationFound = false;
		int maxIteration = 100;
		int iteration = 0;
		while(!locationFound)
		{
			if(iteration == 100)
			{
				Print("Could not find safe position", LogLevel.ERROR);
				return location;
			}

			iteration += 1;
			queriedEntities = {};
			location = RandomPointInRadius(maxDistance) + center;
			location[1] = world.GetSurfaceY(location[0], location[2]) + objDistance + 0.001;
			world.QueryEntitiesBySphere(location, objDistance, FindSafePositionQueryCallback);
			if(queriedEntities.Count() == 0)
				locationFound = true;
		}
		
		return location;
	}
};