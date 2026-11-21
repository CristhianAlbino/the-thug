using UnrealBuildTool;

public class TheThug : ModuleRules
{
	public TheThug(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// The engine-independent rules in /Tools (HunterCore, PerceptionCore) use std exceptions
		// and anonymous-namespace helpers with clashing names, so keep them in separate TUs.
		bEnableExceptions = true;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AIModule"
		});
	}
}
