// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DebugUI : ModuleRules
{
    public DebugUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "Core", "CoreUObject", "Engine", "InputCore",  "UMG"
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "Slate"
                , "SlateCore"
                , "EnhancedInput"
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "InputCore",  "UMG"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "EnhancedInput"
                //, "Sockets"
				//, "Networking"  //네트워크를 직접 다루게 되면 주석 해제
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
