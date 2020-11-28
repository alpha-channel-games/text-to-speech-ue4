using UnrealBuildTool;
using System;
using System.IO;


namespace UnrealBuildTool.Rules
{
	public class TextToSpeech : ModuleRules
	{
		public TextToSpeech(ReadOnlyTargetRules Target) : base(Target)
		{

			PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Media"
			});

            string architecture = Target.Platform == UnrealTargetPlatform.Win64 ? "x64" : "x86";

            // sapihelper.h requires ATL
            PublicAdditionalLibraries.Add(Target.WindowsPlatform.ToolChainDir + @"\atlmfc\lib\" + architecture + @"\atls.lib");
            PrivateIncludePaths.Add(Target.WindowsPlatform.ToolChainDir + @"\atlmfc\include");

            PrivatePCHHeaderFile = "TextToSpeechPCH.h";
        }
	}
}