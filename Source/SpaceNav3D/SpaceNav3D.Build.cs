// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpaceNav3D : ModuleRules
{
	public SpaceNav3D(TargetInfo Target)
	{

        PrivateIncludePathModuleNames.Add("TargetPlatform");
        
        string ThreeDxWareSDKDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "3DxWare";

        // Ensure correct include and link paths for xinput so the correct dll is loaded (xinput1_3.dll)
        PublicSystemIncludePaths.Add(ThreeDxWareSDKDir + "/Inc");
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicLibraryPaths.Add(ThreeDxWareSDKDir + "/Lib/x64");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            PublicLibraryPaths.Add(ThreeDxWareSDKDir + "/Lib/x86");
        }
        PublicAdditionalLibraries.Add("siapp.lib");
        
        PublicIncludePaths.AddRange(
			new string[] {
				"SpaceNav3D/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"SpaceNav3D/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
    			"InputDevice",
				"Slate",
				"SlateCore",
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
