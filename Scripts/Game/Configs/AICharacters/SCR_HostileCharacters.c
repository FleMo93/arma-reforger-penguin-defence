[BaseContainerProps(configRoot:true)]
class SCR_HostileCharacters
{
	[Attribute("", UIWidgets.Object)]
	protected ref array<ref SCR_HostileCharacter> hostileCharacters;
	
    void GetHostileCharacters(array<ref SCR_HostileCharacter> characters)
	{
		if(!characters) return;
		
		foreach(SCR_HostileCharacter character : hostileCharacters)
			characters.Insert(character);
	}
	
	void GetHostileCharactersByRelatedWaveName(array<ref SCR_HostileCharacter> characters, string waveName)
	{
		if(!characters) return;
		
		foreach(SCR_HostileCharacter character : hostileCharacters)
			if(character.GetRelatesToWave() == waveName)
				characters.Insert(character);
	}
}