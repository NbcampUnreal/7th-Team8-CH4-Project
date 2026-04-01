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
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"Heist",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
