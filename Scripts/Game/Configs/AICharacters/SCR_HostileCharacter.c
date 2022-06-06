[BaseContainerProps()]
class SCR_HostileCharacter
{
	[Attribute(params: "et")]
	protected ResourceName itemResourceName;
	
	[Attribute("Default", uiwidget: UIWidgets.EditBox)]
	protected string relatesToWave;
	
	private ref Resource resource;
	
	ResourceName GetResourceName()
	{
		return itemResourceName;
	}
	
	Resource GetResource()
	{
		if(!resource)
			resource = Resource.Load(itemResourceName);
		
		return resource;
	}
	
	string GetRelatesToWave()
	{
		return relatesToWave;
	}
}