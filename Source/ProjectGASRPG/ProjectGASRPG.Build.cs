// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectGASRPG : ModuleRules
{
	public ProjectGASRPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities","AnimGraphRuntime" });
		
		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "HeadMountedDisplay", "NavigationSystem", "Niagara", "GameplayTags", "GameplayTasks", "AIModule"});
	}
}
