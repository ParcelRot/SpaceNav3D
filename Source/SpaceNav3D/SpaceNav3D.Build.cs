// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class SpaceNav3D : ModuleRules
{
    // Helper function to get the Plugin Path folder path
    private string PluginPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "..")); }
    }

    // Helper function to get the Third Party folder path
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(PluginPath, "ThirdParty")); }
    }

    public SpaceNav3D(ReadOnlyTargetRules Target) : base(Target)
	{
        // tanis - start faster compile time for small projects
        MinFilesUsingPrecompiledHeaderOverride = 1;
        bFasterWithoutUnity = true;
        // tanis - end

        PublicIncludePaths.AddRange(
            new string[] {
                "SpaceNav3D/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                "SpaceNav3D/Private"
            }
        );

        BuildVersion Version;
        // Have to use GetDefaultFileName() here because UE 4.18 removed the TryRead member that took one argument from UE 4.17.
        if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
        {
            if (Version.MajorVersion == 4 && Version.MinorVersion <= 17)
            {
                PublicDependencyModuleNames.AddRange(
                    new string[]
                    {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "SlateCore",
                    "Slate",
                    "InputCore",
                    "InputDevice"
                        // ... add other public dependencies that you statically link with here ...
                    }
                );
            }
            else
            {
                PublicDependencyModuleNames.AddRange(
                    new string[]
                    {
                    "Core",
                    "ApplicationCore",
                    "CoreUObject",
                    "Engine",
                    "SlateCore",
                    "Slate",
                    "InputCore",
                    "InputDevice"
                        // ... add other public dependencies that you statically link with here ...
                    }
                );
            }
        }

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add private dependencies that you statically link with here ...
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        // Load the 3DxWare SDK
        Load3DxWareSDK(Target);

    }

    /**
     * Loads the 3DxWare SDK from the plugin's third party folder
     */
    private bool Load3DxWareSDK(ReadOnlyTargetRules Target)
    {
        // Test for compatability
        if (!(Target.Platform == UnrealTargetPlatform.Win64) && !(Target.Platform == UnrealTargetPlatform.Win32))
        {
            return false;
        }

        // Build SDK path
        string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
        string ThreeDeexWareSDKDir = Path.Combine(ThirdPartyPath, "3DxWare SDK");

        // Add .libs
        PublicLibraryPaths.Add(Path.Combine(ThreeDeexWareSDKDir, "Lib", PlatformString));
        PublicAdditionalLibraries.Add("siapp.lib");

        // Add include paths
        PublicIncludePaths.Add(Path.Combine(ThreeDeexWareSDKDir, "Inc"));
        PrivateIncludePathModuleNames.Add("TargetPlatform");

        return true;
    }
}
