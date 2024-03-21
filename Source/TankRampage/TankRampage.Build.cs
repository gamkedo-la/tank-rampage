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
            "MoviePlayer", // Loading Screen
        });

		PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "TRCore",
            "TRTank",
            "TRPlayer",
            "TRAI",
            "TRItem",
            "TRUI",
            "TRGameplayMechanics",
        });

        CppStandard = CppStandardVersion.Cpp20;
    }
}
