[BaseContainerProps(configRoot:true)]
class SCR_WaveTypes
{
	[Attribute("", UIWidgets.Object)]
	protected ref array<ref SCR_WaveType> waveTypes;

	void GetWaveTypes(array<ref SCR_WaveType> wavesArray)
	{
		if(!wavesArray) return;
		
		foreach(SCR_WaveType waveType : waveTypes)
			wavesArray.Insert(waveType);
	}
	
	void GetWaveTypesBySpecial(array<ref SCR_WaveType> wavesArray, bool isSpecial)
	{
		if(!wavesArray) return;
		
		foreach(SCR_WaveType waveType : waveTypes)
		{
			if(waveType.GetIsSepcialWave() == isSpecial)
				wavesArray.Insert(waveType);
		}
	}
}