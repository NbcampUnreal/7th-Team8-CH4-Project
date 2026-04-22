using UnrealBuildTool;

public class HeistUI : ModuleRules
{
	public HeistUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"GameplayAbilities",
			"Heist",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"MultiplayerSessions",
			"OnlineSubsystem",
			"ApplicationCore",
		});
	}
}
