//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Defence", description: "Handles the respawn time.")]
class SCR_DefenceRespawnTimerComponentClass: SCR_RespawnTimerComponentClass
{
};

class SCR_DefenceRespawnTimerComponent : SCR_RespawnTimerComponent
{
	override void OnWorldPostProcess(World world)
	{
		SCR_DefenceGameMode gameMode = SCR_DefenceGameMode.Cast(GetGame().GetGameMode());
		gameMode.GetOnWavePrepare().Insert(ResetTimers);
	}

	protected void ResetTimers()
	{
		foreach (SCR_RespawnTimer timer : m_mRespawnTimers)
			timer.Start(0);
	}
}