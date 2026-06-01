// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PfaffMathisZombieRuntime : ModuleRules
{
	public PfaffMathisZombieRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		bUseRTTI = true;
		bEnableExceptions = true;
		
		PublicIncludePaths.AddRange(
			new string[] {
				ModuleDirectory
			});
				
		PrivateIncludePaths.AddRange(
			new string[] {
			});
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameAI_Zombie",
			});
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AIModule",
				"NavigationSystem",
				"ModularGameplay",
			});
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			});
	}
}