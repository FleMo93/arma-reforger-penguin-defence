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
	private FactionManager factionManager;
	private ArmaReforgerScripted game;
	private World _world;
	private PlayerManager playerManager;
	private vector _center;
	private float _gameAreaRadius;
	private float _defenceRadius;
	private ref Faction hostileAffiliatedFaction;
	
	[Attribute(params: "conf", category: "Loot Points")]
	protected ResourceName waveDifficultyOptionsConfig;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "", "et")]
	protected ResourceName groupWrapperResourceName;
	
	[Attribute(params: "conf")]
	protected ResourceName waveTypesConfig;
	
	[Attribute(params: "conf")]
	protected ResourceName hostileCharactersConfig;
	
	[Attribute("25", uiwidget: UIWidgets.Slider)]
	protected float spawnDistanceOffset;
	
	[Attribute("USSR", uiwidget: UIWidgets.EditComboBox)]
	protected FactionKey hostileAffiliatedFactionKey;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "", "et")]
	protected ResourceName waypointResourceName;
	
	[Attribute("360", uiwidget: UIWidgets.EditComboBox)]
	protected int numberOfSpawnPoints;
	
	[Attribute("1", params: "0.1, 10, 1", uiwidget: UIWidgets.Slider, desc: "")]
	protected float baseHostileMultiplier;

	[Attribute("1", params: "0, 2, 0.5", uiwidget: UIWidgets.Slider, desc: "")]
	protected float extraHostileMultiplierPerPlayer;
	
	protected ref SCR_WaveDifficultyOptionsConfig _waveDifficultyOptionsConfig;
	protected ref SCR_WaveTypes _waveTypesConfig;
	protected ref SCR_HostileCharacters _hostileCharacterConfig;
	protected AIWaypoint aiWaypoint;
	protected ref array<vector> spawnPoints = {};
	protected ref Resource groupWrapperResource;
	protected ref array<SCR_AIGroup> activeGroups = {};
	protected int killedGroupsInCurrentWave = 0;
	protected ref ScriptInvoker onAllGroupsKilled = new ScriptInvoker();
	
	ScriptInvoker GetOnAllGroupsKilled()
	{
		return onAllGroupsKilled;
	}

	override event void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if(!Replication.IsServer())
			return;
		
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
		if(!gameMode) Print("Could not parse game mode", LogLevel.ERROR);

		game = GetGame();
		hostileAffiliatedFaction = game.GetFactionManager().GetFactionByKey(hostileAffiliatedFactionKey);
		
		_world = world;
		playerManager = game.GetPlayerManager();

		if(waveDifficultyOptionsConfig)
			_waveDifficultyOptionsConfig = SCR_WaveDifficultyOptionsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(Resource.Load(waveDifficultyOptionsConfig).GetResource().ToBaseContainer()));
		else
			Print("Could not load wave difficulty options config");
		
		gameMode.GetOnDefenceZoneChanged().Insert(SetDefenceZone);
		gameMode.GetOnWaveStart().Insert(OnWaveStart);
		groupWrapperResource = Resource.Load(groupWrapperResourceName);
		
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
	
	protected SCR_AIGroup SpawnHostileCharacterAtRandomLocation(SCR_HostileCharacter hostileCharacter, EAISkill skill)
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

		IEntity entityGroup = game.SpawnEntityPrefab(groupWrapperResource, _world, spawnParams);
		SCR_AIGroup group =  SCR_AIGroup.Cast(entityGroup);
		if(!group)
		{
			Print(string.Format("Entity of type %1 is no AIGroup", groupWrapperResourceName), LogLevel.ERROR);
			entityGroup.GetParent().RemoveChild(entityGroup);
			return null;
		}

		IEntity entity = game.SpawnEntityPrefab(hostileCharacter.GetResource(), _world, spawnParams);
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if(!character)
		{
			Print(string.Format("Entity of type %1 is no ChimeraCharacter", hostileCharacter.GetResourceName()), LogLevel.ERROR);
			entity.GetParent().RemoveChild(entity);
			return null;
		}
		
		group.AddAIEntityToGroup(entity, 0);
		SetCharacterSkill(character, skill);
		SetGroupAffiliatedFaction(group);
		SetGroupWaypoint(group);
		
		return group;
	}

	protected void SetGroupWaypoint(SCR_AIGroup group)
	{
		array<AIWaypoint> activeWaypoints = {};
		group.GetWaypoints(activeWaypoints);
		foreach(AIWaypoint wpt : activeWaypoints)
			group.RemoveWaypoint(wpt);

		group.AddWaypoint(aiWaypoint);
	}
	
	protected void SetGroupAffiliatedFaction(SCR_AIGroup group)
	{
		group.SetFaction(hostileAffiliatedFaction);
	}
	
	protected void SetCharacterSkill(SCR_ChimeraCharacter character, EAISkill skill)
	{
		Managed managed = character.FindComponent(SCR_AICombatComponent);		
		SCR_AICombatComponent component = SCR_AICombatComponent.Cast(managed);
		if(!component)
		{
			Print("SCR_AICombatComponent not found", LogLevel.ERROR);
			return;
		}
		
		component.SetAISkill(skill);
	}
	
	protected void OnGroupEmpty()
	{
		killedGroupsInCurrentWave++;
		if(killedGroupsInCurrentWave < activeGroups.Count())
			return;
		
		for(int i = 0; i < activeGroups.Count(); i++)
		{
			SCR_AIGroup group = activeGroups[i];
			delete group;
		}
		activeGroups.Clear();
		onAllGroupsKilled.Invoke();
	}
	
	protected void OnWaveStart()
	{
		killedGroupsInCurrentWave = 0;
		array<ref SCR_WaveType> waveTypes = {};
		_waveTypesConfig.GetWaveTypes(waveTypes);
		int waveTypeIndex = Math.RandomInt(0, waveTypes.Count());
		SCR_WaveType waveType = waveTypes[waveTypeIndex];
		array<ref SCR_HostileCharacter> hostileCharacters = {};
		_hostileCharacterConfig.GetHostileCharactersByRelatedWaveName(hostileCharacters, waveType.GetName());
		EAISkill skill = GetHostileSkill();
		
		for(int i = 0; i < GetEnemyAmount(); i++)
		{
			SCR_HostileCharacter hostileCharacter = hostileCharacters[Math.RandomInt(0, hostileCharacters.Count())];
			SCR_AIGroup group = SpawnHostileCharacterAtRandomLocation(hostileCharacter, skill);
			activeGroups.Insert(group);
			group.GetOnEmpty().Insert(OnGroupEmpty);
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
		return spawnPoints[Math.RandomInt(0, spawnPoints.Count())];
	}
	
	protected int GetEnemyAmount()
	{
		int extraPlayerCountMultipier = playerManager.GetPlayerCount() - 1;
		float playerMultiplier = 1 + (extraPlayerCountMultipier * extraHostileMultiplierPerPlayer);
		int numberOfHostiles = Math.Floor(gameMode.GetCurrentWave() * playerMultiplier * baseHostileMultiplier);
		return Math.Max(numberOfHostiles, 1);
	}
	
	protected EAISkill GetHostileSkill()
	{
		SCR_WaveDifficultyOption option = _waveDifficultyOptionsConfig.GetWaveDifficultyOptionForWave(gameMode.GetCurrentWave());
		return option.GetAISkill();
	}
	
	
	#ifdef WORKBENCH
	//! Possibility to get variable value choices dynamically
	override array<ref ParamEnum> _WB_GetUserEnums(string varName, IEntity owner, IEntityComponentSource src)
	{
		if (varName == "hostileAffiliatedFaction")
		{
			FactionManager manager = GetGame().GetFactionManager();
			if (manager)
			{
				ref array<ref ParamEnum> factionEnums = new array<ref ParamEnum>();
				factionEnums.Insert(new ParamEnum("Disabled", "-1"));

				Faction faction;
				string name;
				int factionCount = manager.GetFactionsCount();
				for (int i = 0; i < factionCount; i++)
				{
					faction = manager.GetFactionByIndex(i);
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