[BaseContainerProps(configRoot:true)]
class SCR_LootListConfig
{
	[Attribute("", UIWidgets.Object)]
	protected ref array<ref SCR_LootGroup> lootGroups;
	
	private int totalWeight = 0;
	
	void SCR_LootListConfig()
	{
		totalWeight = 0;
		foreach(SCR_LootGroup lootGroup : lootGroups)
			totalWeight = totalWeight + lootGroup.GetWeight();
	}
	
	void GetLootGroups(array<ref SCR_LootGroup> groups)
	{
		if(!groups) return;
		
		foreach(SCR_LootGroup group : lootGroups)
			groups.Insert(group);
	}
	
	SCR_LootGroup GetRandomLootGroupByWeight()
	{
		float pickedWeight = Math.RandomInt(0, totalWeight + 1);
		int currentWeight = 0;
		
		foreach(SCR_LootGroup group : lootGroups)
		{
			currentWeight = currentWeight + group.GetWeight();
			if(pickedWeight <= currentWeight)
				return group;
		}
		
		return null;
	}
	
	SCR_LootGroup GetRandomLootGroup()
	{
		float pickedWeight = Math.RandomInt(0, lootGroups.Count());
		int currentWeight = 0;
		
		foreach(SCR_LootGroup group : lootGroups)
		{
			currentWeight += 1;
			if(pickedWeight <= currentWeight)
				return group;
		}
		
		return null;
	}
}