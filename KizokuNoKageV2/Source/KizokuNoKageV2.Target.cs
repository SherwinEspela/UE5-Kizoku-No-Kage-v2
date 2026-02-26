// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class KizokuNoKageV2Target : TargetRules
{
	public KizokuNoKageV2Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "KizokuNoKageV2" } );

        WindowsPlatform.bStrictInlineConformance = false;

        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        //UndefinedIdentifierWarningLevel = WarningLevel.Off;

        BuildEnvironment = TargetBuildEnvironment.Shared;
        bOverrideBuildEnvironment = true;
    }
}
