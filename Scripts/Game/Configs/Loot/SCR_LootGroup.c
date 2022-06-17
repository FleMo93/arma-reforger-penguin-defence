[BaseContainerProps()]
class SCR_LootGroup
{
	[Attribute("", uiwidget: UIWidgets.EditBox, desc: "Name of the group (optional)")]
	protected string name;
	
	[Attribute("1000", params: "0, 1000000, 1", uiwidget: UIWidgets.Slider)]
	protected int weight;
	
	[Attribute("1", uiwidget: UIWidgets.Slider, params: "1, 5, 1", desc: "Amount of the same item spawned")]
	protected int batchSize;

	[Attribute("", UIWidgets.Object)]
	protected ref array<ref SCR_LootItem> lootItems;
	
	private int totalWeight = 0;
	
	void SCR_LootGroup()
	{
		totalWeight = 0;
		foreach(SCR_LootItem lootItem : lootItems)
			totalWeight += lootItem.GetWeight();
	}
	
	string GetName()
	{
		return name;
	}
	
	int GetWeight()
	{
		return weight;
	}
	
	int GetBatchSize()
	{
		return batchSize;
	}
	
	void GetLootItems(array<ref SCR_LootItem> items)
	{
		if(!items) return;
		
		foreach(SCR_LootItem lootItem : items)
			items.Insert(lootItem);
	}
	
	SCR_LootItem GetRandomLootItemByWeight()
	{
		float pickedWeight = Math.RandomInt(0, totalWeight + 1);
		int currentWeight = 0;
		
		foreach(SCR_LootItem item : lootItems)
		{
			currentWeight += item.GetWeight();
			if(pickedWeight <= currentWeight)
				return item;
		}
		
		return null;
	}
	
	SCR_LootItem GetRandomLootItem()
	{
		float pickedWeight = Math.RandomInt(0, lootItems.Count());
		int currentWeight = 0;
		
		foreach(SCR_LootItem item : lootItems)
		{
			currentWeight += 1;
			if(pickedWeight <= currentWeight)
				return item;
		}
		
		return null;
	}
}