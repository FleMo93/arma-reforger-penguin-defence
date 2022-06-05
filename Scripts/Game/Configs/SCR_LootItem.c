[BaseContainerProps()]
class SCR_LootItem
{
	[Attribute(params: "et")]
	protected ResourceName itemResourceName;
	
	[Attribute("1", uiwidget: UIWidgets.Slider)]
	protected int weight;
	
	private ref Resource resource;
	
	ResourceName GetResourceName()
	{
		return itemResourceName;
	}
	
	int GetWeight()
	{
		return weight;
	}
	
	Resource GetResource()
	{
		if(!resource)
			resource = Resource.Load(itemResourceName);
		
		return resource;
	}
}