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
            "GameplayAbilities", // GAS
			"GameplayTags", // GAS
			"GameplayTasks", // GAS
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
            "TRSettings",
                
            // Controller detection support in TRGameInstance.cpp
            "Slate",
            "SlateCore",
            "ApplicationCore",
        });

        CppStandard = CppStandardVersion.Cpp20;
    }
}
