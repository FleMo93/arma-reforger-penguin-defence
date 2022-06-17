[BaseContainerProps()]
class SCR_WaveDifficultyOption
{	
	[Attribute("1", params: "1, 100000, 1", uiwidget: UIWidgets.Slider, desc: "Active until wave. Wave number included.")]
	protected int waveNumber;
	
	[Attribute("50", UIWidgets.ComboBox, "AI skill in combat", "", ParamEnumArray.FromEnum(EAISkill) )]
	protected EAISkill aiSkill;

	int GetWaveNumber()
	{
		return waveNumber;
	}
	
	EAISkill GetAISkill()
	{
		return aiSkill;
	}
}