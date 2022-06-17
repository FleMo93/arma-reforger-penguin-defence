[BaseContainerProps()]
class SCR_WaveType
{
	[Attribute("Default", uiwidget: UIWidgets.EditBox)]
	protected string name;
	
	[Attribute("false", uiwidget: UIWidgets.CheckBox)]
	protected bool isSpecialWave;
	
	string GetName()
	{
		return name;
	}
	
	bool GetIsSepcialWave()
	{
		return isSpecialWave;
	}
}