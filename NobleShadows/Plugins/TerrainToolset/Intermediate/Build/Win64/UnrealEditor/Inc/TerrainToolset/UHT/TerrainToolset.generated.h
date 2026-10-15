// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "TerrainToolset.h"

#ifdef TERRAINTOOLSET_TerrainToolset_generated_h
#error "TerrainToolset.generated.h already included, missing '#pragma once' in TerrainToolset.h"
#endif
#define TERRAINTOOLSET_TerrainToolset_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class ALandscape;
class ALandscapeProxy;
class UMaterialInterface;

// ********** Begin Class UTerrainToolset **********************************************************
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execReadLayerBlobs); \
	DECLARE_FUNCTION(execPaintZoneMap); \
	DECLARE_FUNCTION(execCreateLayerInfos); \
	DECLARE_FUNCTION(execLoadWorldRegion); \
	DECLARE_FUNCTION(execImportHeightmapFile); \
	DECLARE_FUNCTION(execAddNoise); \
	DECLARE_FUNCTION(execAddHill); \
	DECLARE_FUNCTION(execSetHeights); \
	DECLARE_FUNCTION(execGetHeights); \
	DECLARE_FUNCTION(execSetFlat); \
	DECLARE_FUNCTION(execPlanLandscape); \
	DECLARE_FUNCTION(execGetLandscapeInfo); \
	DECLARE_FUNCTION(execCreateLandscape);


struct Z_Construct_UClass_UTerrainToolset_Statics;
TERRAINTOOLSET_API UClass* Z_Construct_UClass_UTerrainToolset(ETypeConstructPhase);

#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UTerrainToolset_Statics; \
	friend TERRAINTOOLSET_API UClass* ::Z_Construct_UClass_UTerrainToolset(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UTerrainToolset, UToolsetDefinition, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/TerrainToolset"), Z_Construct_UClass_UTerrainToolset) \
	DECLARE_SERIALIZER(UTerrainToolset)


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UTerrainToolset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UTerrainToolset(UTerrainToolset&&) = delete; \
	UTerrainToolset(const UTerrainToolset&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UTerrainToolset); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UTerrainToolset); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UTerrainToolset) \
	NO_API virtual ~UTerrainToolset();


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_13_PROLOG
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_INCLASS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UTerrainToolset;

// ********** End Class UTerrainToolset ************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
