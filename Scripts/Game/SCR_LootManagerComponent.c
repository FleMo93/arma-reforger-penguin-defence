class FloatCluster
{
	float start, end;
	ref array<float> entries = {};

	void FloatCluster(float clusterStart, float clusterEnd)
	{
		start = clusterStart;
		end = clusterEnd;
	}
}

class LootBuilding
{
	ref array<ref vector> spawnPoints = {};

	protected ref array<float> heights = {};

	private IEntity _building;
	private BaseWorld _world;
	private vector _mins, _maxs;
	private vector transformationBuilding[4];
	private float __heightClusterRange;
	
	void LootBuilding(BaseWorld world, IEntity building, float _heightClusterRange)
	{
		_building = building;
		_world = world;
		_building.GetBounds(_mins, _maxs);
		_building.GetWorldTransform(transformationBuilding);
		__heightClusterRange = _heightClusterRange;

		FillLootHeights();
		ClusterHeights();
		CalculateSpawnPoints();
		
	}
	
	protected bool DoorQueryEntitiesCallbackFilter(IEntity e)
	{
		return true;
	}
	
	protected bool DoorQueryEntitiesCallbackAdd(IEntity e)
	{
		array<Managed> doorComponents = {};
		e.FindComponents(DoorComponent, doorComponents);
		if(doorComponents.Count() == 0)
			return true;
		
		vector transformation[4];
		e.GetWorldTransform(transformation);
		float height = transformation[3][1];
		if(heights.Find(height) == -1)
			heights.Insert(height);
		return true;
	}
	
	private FloatCluster FindCluster(array<ref FloatCluster> clusters, float value)
	{
		foreach(FloatCluster cluster : clusters)
		{
			if(cluster.start <= value && cluster.end >= value)
				return cluster;
		}
		
		return null;
	}
	
	protected void ClusterHeights() 
	{
		array<ref FloatCluster> clusters = {};
		heights.Sort();

		foreach(float height : heights)
		{
			FloatCluster cluster = FindCluster(clusters, height);
			if(!cluster)
			{
				cluster = new FloatCluster(height, height + __heightClusterRange);
				clusters.Insert(cluster);
			}
		}
		
		heights.Clear();
		foreach(FloatCluster cluster : clusters)
		{
			heights.Insert(cluster.start);
		}
		delete clusters;
	}
	
	protected void FillLootHeights()
	{
		_world.QueryEntitiesByOBB(_mins, _maxs, transformationBuilding, DoorQueryEntitiesCallbackAdd, DoorQueryEntitiesCallbackFilter);
	}
	
	protected void CalculateSpawnPoints()
	{
		vector size = Vector(_maxs[0] - _mins[0], 0, _maxs[2] - _mins[2]);
		vector sizeHalf = Vector(size[0] / 2, 0, size[2] / 2);
		vector start = Vector(1, 0, 1);
		vector end = Vector(size[0] - 1, 0, size[2] - 1);
		int xPoints = Math.Floor(size[0] - 2);
		int yPoints = Math.Floor(size[2] - 2);
		array<ref vector> relativeSpawnPoints = {};

		for(int x = 0; x <= xPoints; x++)
		{
			for(int y = 0; y <= yPoints; y++)
			{
				vector relativeSpawnPoint = Vector(
					start[0] + x - sizeHalf[0],
					0,
					start[2] + y - sizeHalf[2]);
				relativeSpawnPoint = relativeSpawnPoint.Multiply4(transformationBuilding);
				relativeSpawnPoints.Insert(relativeSpawnPoint);
			}
		}

		foreach(float height : heights)
		{
			foreach(vector relativeSpawnPoint : relativeSpawnPoints)
			{
				vector absoluteSpawnPoint = Vector(relativeSpawnPoint[0], height, relativeSpawnPoint[2]);
				spawnPoints.Insert(absoluteSpawnPoint);
			}
		}
		
		delete relativeSpawnPoints;
	}
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Defence", description: "Handles the loot.")]
class SCR_DefenceLootManagerComponentClass: SCR_BaseGameModeComponentClass
{
};



//------------------------------------------------------------------------------------------------
/*
	Component responsible for handling automatic spawning logic.
*/
class SCR_DefenceLootManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("2", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float spawnHeightOffset;

	[Attribute("0.1", uiwidget: UIWidgets.Slider, params: "0.01, 1.0, 0.01", category: "Loot Points", desc: "")]
	protected float chanceOfSpawnPerPoint;
	
	[Attribute("2", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float heightClusterRange;
	
	[Attribute(params: "conf", category: "Loot Points")]
	protected ResourceName lootListConfig;
	
	[Attribute(params: "conf", category: "Loot Points")]
	protected bool ignoreItemListWeights;
	
	protected ref SCR_LootListConfig _lootListConfig;
	
	protected BaseGameTriggerEntity defendPointTrigger;
	protected ref array<ref LootBuilding> lootBuildings = {};
	protected ref array<ref SCR_ArsenalItem> items  = {};
	private World _world;
	private ArmaReforgerScripted game;
	protected ref array<GenericEntity> spawnedEntities = {};
	protected ref array<GenericEntity> ownedItems = {};
	private ref RandomGenerator randomGenerator;
	protected vector _center;
	private SCR_DefenceGameMode gameMode;
	private float gameAreaRadius;
	
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if(!Replication.IsServer())
			return;

		_world = world;
		game = GetGame();
		
		randomGenerator = new RandomGenerator();
		randomGenerator.SetSeed(Math.RandomInt(0, 100));
		
		gameMode = SCR_DefenceGameMode.Cast(GetGameMode());
		if(!gameMode) Print("Could not parse game mode", LogLevel.ERROR);
		
		Resource lootList = Resource.Load(lootListConfig);
		Managed lootBaseContainer =  BaseContainerTools.CreateInstanceFromContainer(lootList.GetResource().ToBaseContainer());
		_lootListConfig = SCR_LootListConfig.Cast(lootBaseContainer);
		
		gameMode.GetOnDefenceZoneChanged().Insert(SetDefenceZone);
		gameMode.GetOnWavePrepare().Insert(SpawnLoot);
	}
	
	protected bool BuildingQueryEntitiesCallbackFilter(IEntity e)
	{
		return true;
	}
	
	protected bool BuildingQueryEntitiesCallbackAdd(IEntity e)
	{
		SCR_DestructibleBuildingEntity genericEntity = SCR_DestructibleBuildingEntity.Cast(e);
		if(!genericEntity)
			return true;
		LootBuilding lootBuilding = new LootBuilding(_world, e, heightClusterRange);
		if(lootBuilding.spawnPoints.Count() > 0)
			lootBuildings.Insert(lootBuilding);
		
		return true;
	}
	
	protected void SetDefenceZone(vector center, float radius)
	{
		_center = center;
		gameAreaRadius = radius;
		GenerateSpawnPoints();
	}
	
	protected void GenerateSpawnPoints()
	{	
		_world.QueryEntitiesBySphere(_center, gameAreaRadius, BuildingQueryEntitiesCallbackAdd, BuildingQueryEntitiesCallbackFilter, EQueryEntitiesFlags.STATIC);
	}
	

	protected void SpawnLootAt(vector position, array<GenericEntity> newEntities)
	{
		EntitySpawnParams spawnParams = EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = {
			Vector(1, 0, 0),
			Vector(0, 1, 0),
			Vector(0, 0, 1),
			Vector(position[0], position[1] /*+ _spawnHeightOffset*/, position[2])
		};
				
		_lootListConfig.GetRandomLootGroupByWeight(randomGenerator);

		SCR_LootGroup lootGroup;
		if (ignoreItemListWeights)
			lootGroup = _lootListConfig.GetRandomLootGroup(randomGenerator);
		else
			lootGroup = _lootListConfig.GetRandomLootGroupByWeight(randomGenerator);

		if(!lootGroup)
		{
			Print("No loot group could be found", LogLevel.ERROR);
			return;
		}

		SCR_LootItem lootItem;
		if(ignoreItemListWeights)
			lootItem = lootGroup.GetRandomLootItem(randomGenerator);
		else
			lootItem = lootGroup.GetRandomLootItemByWeight(randomGenerator);

		if(!lootItem)
		{
			Print("No loot item could be found", LogLevel.ERROR);
			return;
		}
		
		for(int i = 0; i < lootGroup.GetBatchSize(); i++)
		{
			Resource itemResource = lootItem.GetResource();		
			IEntity entity = game.SpawnEntityPrefab(itemResource, _world, spawnParams);
			GenericEntity gEntity = GenericEntity.Cast(entity);
			if(!gEntity)
			{
				Print(string.Format("Could not cast entity %1", lootItem.GetResourceName()), LogLevel.ERROR);
				delete entity;
				continue;
			}
			
			//leads to strange behaviour, like weapons flying around when stucking in furniture or walls
			//Physics physics = entity.GetPhysics();
			//if(physics)
			//{
				//physics.Destroy();
				//physics = Physics.CreateDynamic(entity, 1, -1);
			//}
			
			newEntities.Insert(gEntity);
		}
	}
	
	protected void CleanUpLoot()
	{
		array<GenericEntity> newOwnedItems = {};
		foreach (GenericEntity lootItem : ownedItems)
		{
			if(!lootItem)
				continue;

			IEntity parent = lootItem.GetParent();
			if(!parent)
				delete lootItem;
			else
				newOwnedItems.Insert(lootItem);
		}

		foreach (GenericEntity lootItem : spawnedEntities)
		{
			if(!lootItem)
				continue;
			IEntity parent = lootItem.GetParent();
			if(!parent)
				delete lootItem;
			else
				newOwnedItems.Insert(lootItem);
		}

		ownedItems = newOwnedItems;
		spawnedEntities = {};
	}
	
	protected void SpawnLoot()
	{
		CleanUpLoot();
		
		foreach(LootBuilding lootBuilding : lootBuildings)
		{
			foreach(vector spawnPoint : lootBuilding.spawnPoints)
			{
				if(randomGenerator.RandFloat01() > chanceOfSpawnPerPoint)
					continue;

				SpawnLootAt(spawnPoint, spawnedEntities);
			}
		}
	}
}