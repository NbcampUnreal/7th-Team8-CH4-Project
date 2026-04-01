// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Heist : ModuleRules
{
	public Heist(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
			"ModularGameplay",
			"MultiplayerSessions",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
