[BaseContainerProps(configRoot:true)]
class SCR_WaveDifficultyOptionsConfig
{
	[Attribute("", UIWidgets.Object, desc: "Must be sorted.")]
	protected ref array<ref SCR_WaveDifficultyOption> waveDifficultyOptions;

	SCR_WaveDifficultyOption GetWaveDifficultyOptionForWave(int waveNumber)
	{
		foreach(SCR_WaveDifficultyOption option : waveDifficultyOptions)
			if(waveNumber <= option.GetWaveNumber())
				return option;

		return waveDifficultyOptions[waveDifficultyOptions.Count() - 1];
	}
}