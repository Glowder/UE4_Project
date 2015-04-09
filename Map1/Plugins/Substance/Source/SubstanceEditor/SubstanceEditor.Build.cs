// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SubstanceEditor : ModuleRules
{
	public SubstanceEditor(TargetInfo Target)
	{
		PrivateIncludePaths.Add("SubstanceEditor/Private");

		PublicDependencyModuleNames.AddRange(new string[] {
				"AssetTools",
				"BlueprintGraph",
				"ContentBrowser",
				"Core",
				"CoreUObject",
				"DesktopPlatform",
				"EditorStyle",
				"Engine",
				"InputCore",
				"KismetCompiler",
				"MainFrame",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"ShaderCore",
				"SubstanceCore",
				"TextureEditor",
				"UnrealEd"
			});
			
		PrivateDependencyModuleNames.AddRange(new string[] {
                "AppFramework",
				"Slate",
                "SlateCore",
        });
	}
}
