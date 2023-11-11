// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TankRampage : ModuleRules
{
	public TankRampage(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
        });

		PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "TRCore",
            "TRTank",
        });

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        CppStandard = CppStandardVersion.Cpp20;
    }
}
