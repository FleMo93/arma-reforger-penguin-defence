class SCR_CharacterAreaRestrictionComponentClass: ScriptComponentClass
{
};

/**
* Component teleporting player back to area when leaving
*/
class SCR_CharacterAreaRestrictionComponent: ScriptComponent
{
	private SCR_DefenceGameMode m_gameMode;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
	
	override void EOnInit(IEntity owner)
	{
		if(!Replication.IsServer())
			return;
		
		m_gameMode = SCR_DefenceGameMode.Cast(GetGame().GetGameMode());
	}

	override event protected void EOnFrame(IEntity owner, float timeSlice)
	{
		float distance = vector.Distance(owner.GetOrigin(), m_gameMode.GetGameAreaCenter());
		if(distance > m_gameMode.GetGameAreaRadius())
		{
			BaseGameEntity baseGameEntity = BaseGameEntity.Cast(owner);
			vector target[4];
			owner.GetWorldTransform(target);
			target[3] = m_gameMode.GetGameAreaCenter();
			baseGameEntity.Teleport(target);
		}
	}
}