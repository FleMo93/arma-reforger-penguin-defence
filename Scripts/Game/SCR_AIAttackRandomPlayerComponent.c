class PlayerEntityIdMap
{
	private IEntity m_entity;
	private int m_iId;
	
	IEntity GetEntity()
	{
		return m_entity;
	}
	
	int GetId()
	{
		return m_iId;
	}
	
	void PlayerEntityIdMap(IEntity entity, int id)
	{
		m_entity = entity;
		m_iId = id;
	}
}

[ComponentEditorProps(category: "GameScripted/Defence", description: "Component controlling the waypoints for the ai group. Directly attack a random player.", color: "0 0 255 255")]
class SCR_AIAttackRandomPlayerComponentClass : ScriptComponentClass
{
}

class SCR_AIAttackRandomPlayerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, "", "et")]
	protected ResourceName m_waypointResourceName;
	
	[Attribute("5", params: "0, 600, 0.1", uiwidget: UIWidgets.Slider, desc: "If not player is found to attack the search will be repeated after the given amount of time.")]
	protected float m_fRepeatTime;
	
	private SCR_AIGroup m_aiGroupOwner;
	private PlayerManager m_playerManager;
	private IEntity m_entityTarget;
	private int m_iTargetPlayerId;
	private SCR_BaseGameMode m_baseGameMode;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if(!Replication.IsServer())
			return;

		m_aiGroupOwner = SCR_AIGroup.Cast(owner);
		if(!m_aiGroupOwner)
		{
			Print("Owner must be of type SCR_AIGroup", LogLevel.ERROR);
			return;
		}
		
		m_playerManager = GetGame().GetPlayerManager();
		
		m_baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(!m_baseGameMode)
		{
			Print("GameMode must be of SCR_BaseGameMode", LogLevel.ERROR);
			return;
		}
		
		m_baseGameMode.GetOnPlayerKilled().Insert(OnPlayerKilledOrDisconnected);
		m_baseGameMode.GetOnPlayerDisconnected().Insert(OnPlayerKilledOrDisconnected);

		AttackRandomPlayer();
	}
	
	override protected void OnDelete(IEntity owner)
	{
		m_baseGameMode.GetOnPlayerKilled().Remove(OnPlayerKilledOrDisconnected);
		m_baseGameMode.GetOnPlayerDisconnected().Remove(OnPlayerKilledOrDisconnected);
		GetGame().GetCallqueue().Remove(AttackRandomPlayer);
		super.OnDelete(owner);
	}
	
	void AttackPlayer(IEntity player, int playerId)
	{
		SCR_EntityWaypoint aiWaypoint = CreateWaypointForTarget(player);
		if(!aiWaypoint)
		{
			Print("Could not create ai waypoint", LogLevel.ERROR);
			return;
		}

		if(!aiWaypoint)
			return;

		SetGroupWaypoint(aiWaypoint);

		m_entityTarget = player;
		m_iTargetPlayerId = playerId;
	}
	
	void AttackRandomPlayer()
	{
		PlayerEntityIdMap player = GetRandomPlayer();
		if(!player)
		{
			GetGame().GetCallqueue().CallLater(AttackRandomPlayer, m_fRepeatTime * 1000);
			return;
		}

		AttackPlayer(player.GetEntity(), player.GetId());
	}
	
	private SCR_EntityWaypoint CreateWaypointForTarget(IEntity target)
	{
		Resource resource = Resource.Load(m_waypointResourceName);
		if (!resource)
			return null;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = Vector(0, 0, 0);
		SCR_EntityWaypoint waypoint = SCR_EntityWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, null, spawnParams));
		waypoint.SetEntity(target);
		target.AddChild(waypoint, -1, EAddChildFlags.AUTO_TRANSFORM);
		return waypoint;
	}
	
	private void SetGroupWaypoint(AIWaypoint targetWaypoint)
	{
		array<AIWaypoint> waypoints = {};
		m_aiGroupOwner.GetWaypoints(waypoints);
		foreach(AIWaypoint waypoint : waypoints)
			m_aiGroupOwner.RemoveWaypoint(waypoint);

		m_aiGroupOwner.AddWaypoint(targetWaypoint);
	}

	private PlayerEntityIdMap GetRandomPlayer()
	{
		array<int> playerIds = {};
		m_playerManager.GetAllPlayers(playerIds);

		array<ref PlayerEntityIdMap> playerEntities = {};
		foreach(int playerId : playerIds)
		{
			IEntity playerEntity = m_playerManager.GetPlayerControlledEntity(playerId);
			if(!playerEntity)
				continue;

			CharacterControllerComponent characterControllerComponent = CharacterControllerComponent.Cast(playerEntity.FindComponent(CharacterControllerComponent));
			if(!characterControllerComponent || characterControllerComponent.IsDead())
				continue;

			playerEntities.Insert(new PlayerEntityIdMap(playerEntity, playerId));
		}
		
		if(playerEntities.Count() == 0)
			return null;
		
		return playerEntities[Math.RandomInt(0, playerEntities.Count())];
	}
	
	private void OnPlayerKilledOrDisconnected(int playerId)
	{
		if(playerId != m_iTargetPlayerId)
			return;
		
		AttackRandomPlayer();
	}
}