//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Defence", description: "Handles the loot.")]
class SCR_DefenceLootComponentClass: ScriptGameComponentClass
{
};

class LootBuilding
{
	ref array<ref vector> spawnPoints = {};

	protected ref array<float> heights = {};

	private IEntity _building;
	private BaseWorld _world;
	private vector _mins, _maxs;
	private vector transformationBuilding[4];
	
	void LootBuilding(BaseWorld world, IEntity building)
	{
		_building = building;
		_world = world;
		_building.GetBounds(_mins, _maxs);
		_building.GetWorldTransform(transformationBuilding);

		FillLootHeights();
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
/*
	Component responsible for handling automatic spawning logic.
*/
class SCR_DefenceLootComponent : ScriptGameComponent
{
	[Attribute("", uiwidget: UIWidgets.EditComboBox, category: "Loot Points", desc: "Trigger where players can spawn in.")]
	protected string respawnTriggerEntityName;
	
	[Attribute("100", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float gameAreaRadius;
	
	[Attribute("2", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected float spawnHeightOffset;

	[Attribute("2", uiwidget: UIWidgets.EditBox, category: "Loot Points", desc: "")]
	protected int spawnPointsPerFloor;
	
	[Attribute(uiwidget: UIWidgets.EditBox, category: "Loot Points")]
	protected ref array<ref ResourceName> itemsConfigs;
	
	protected BaseGameTriggerEntity defendPointTrigger;
	protected ref array<ref LootBuilding> lootBuildings = {};
	protected ref array<ref SCR_ArsenalItem> items  = {};
	private BaseWorld world;
	private ArmaReforgerScripted game;
	protected ref array<IEntity> spawnedEntities = {};
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		game = GetGame();
		world = game.GetWorld();
		IEntity respawnTriggerEntity = game.FindEntity(respawnTriggerEntityName);
		defendPointTrigger = BaseGameTriggerEntity.Cast(respawnTriggerEntity);
		if (!defendPointTrigger)
		{
			Print("Respawn trigger entity not found", LogLevel.ERROR);
			return;
		}

		LoadItems();
		GenerateSpawnPoints();
		SpawnLoot();
	}
	
	protected void LoadItems()
	{
		foreach(ResourceName configResourceName : itemsConfigs)
		{
			Resource holder = BaseContainerTools.LoadContainer(configResourceName);
			if (!holder)
				continue;
			
			BaseContainer cont = holder.GetResource().ToBaseContainer();
			if (!cont)
				continue;

			if (cont.GetClassName() != "SCR_ArsenalItemListConfig")
			{
				Print(string.Format("Config '%1' is of type '%2', must be 'SCR_ArsenalItemListConfig'!", configResourceName, cont.GetClassName()), LogLevel.ERROR);
				continue;
			}

			SCR_ArsenalItemListConfig itemsConfig = SCR_ArsenalItemListConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(cont));
			if (!itemsConfig)
				continue;

			array<ref SCR_ArsenalItem> arsenalItems;
			if(!itemsConfig.GetArsenalItems(arsenalItems))
				continue;
			
			foreach(ref SCR_ArsenalItem arsenalItem : arsenalItems)
			{
				if(arsenalItem != null)
					items.Insert(arsenalItem);
			}
		}
	}
	
	protected bool BuildingQueryEntitiesCallbackFilter(IEntity e)
	{
		SCR_DestructibleBuildingEntity genericEntity = SCR_DestructibleBuildingEntity.Cast(e);
		if(!genericEntity)
			return false;
		return true;
	}
	
	protected bool BuildingQueryEntitiesCallbackAdd(IEntity e)
	{
		LootBuilding lootBuilding = new LootBuilding(world, e);
		if(lootBuilding.spawnPoints.Count() > 0)
			lootBuildings.Insert(lootBuilding);
		
		Print(string.Format("Found %1 spawn points in building", lootBuilding.spawnPoints.Count()), LogLevel.WARNING);
		return true;
	}
	
	protected void GenerateSpawnPoints()
	{
		if(!world) return;
		vector triggerLocation[4];
		defendPointTrigger.GetWorldTransform(triggerLocation);		
		if(!world.QueryEntitiesBySphere(triggerLocation[3], gameAreaRadius, BuildingQueryEntitiesCallbackAdd, BuildingQueryEntitiesCallbackFilter, EQueryEntitiesFlags.STATIC))
			return;
	}
	
	protected void SpawnLoot()
	{
		foreach(LootBuilding lootBuilding : lootBuildings)
		{
			foreach(vector spawnPoint : lootBuilding.spawnPoints)
			{
				int index = Math.RandomInt(0, items.Count() - 1);
				SCR_ArsenalItem item = items[index];
				EntitySpawnParams spawnParams = EntitySpawnParams();
				spawnParams.TransformMode = ETransformMode.WORLD;
				spawnParams.Transform = {
					Vector(1, 0, 0),
					Vector(0,1, 0),
					Vector(0, 0, 1),
					Vector(spawnPoint[0], spawnPoint[1] + spawnHeightOffset, spawnPoint[2])
				};
				Resource itemResource = item.GetItemResource();
				IEntity entity = game.SpawnEntityPrefab(itemResource, world, spawnParams);
				spawnedEntities.Insert(entity);
				
				//leads to strange behaviour, like weapons flying around when stucking in furniture or walls
				//Physics physics = entity.GetPhysics();
				//if(physics)
				//{
					//physics.Destroy();
					//physics = Physics.CreateDynamic(entity, 1, -1);
				//}
			}
		}
	}
}