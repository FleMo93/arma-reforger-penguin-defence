[BaseContainerProps()]
class SCR_WaveType
{
	[Attribute("Default", uiwidget: UIWidgets.EditBox)]
	protected string name;
	
	string GetName()
	{
		return name;
	}
}