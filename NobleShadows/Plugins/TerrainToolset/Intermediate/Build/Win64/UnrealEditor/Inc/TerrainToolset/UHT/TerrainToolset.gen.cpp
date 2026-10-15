// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TerrainToolset.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeTerrainToolset() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FRotator(ETypeConstructPhase);
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector(ETypeConstructPhase);
TOOLSETREGISTRY_API UClass* Z_Construct_UClass_UToolsetDefinition(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface(ETypeConstructPhase);
LANDSCAPE_API UClass* Z_Construct_UClass_ALandscape(ETypeConstructPhase);
LANDSCAPE_API UClass* Z_Construct_UClass_ALandscapeProxy(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_TerrainToolset(ETypeConstructPhase);
TERRAINTOOLSET_API UClass* Z_Construct_UClass_UTerrainToolset(ETypeConstructPhase);
TERRAINTOOLSET_API UClass* Z_Construct_UClass_UTerrainToolset(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin Class UTerrainToolset Function AddHill *****************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_AddHill_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventAddHill_Parms
	{
		ALandscapeProxy* Landscape;
		float CenterX;
		float CenterY;
		float RadiusCm;
		float HeightCm;
		float FalloffPower;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Adds (or subtracts, if negative) a smooth round hill.\n\x09 * @param Landscape The landscape.\n\x09 * @param CenterX World X of the hill centre (cm).\n\x09 * @param CenterY World Y of the hill centre (cm).\n\x09 * @param RadiusCm Hill radius in cm.\n\x09 * @param HeightCm Height added at the centre (cm); negative digs.\n\x09 * @param FalloffPower 1 = linear cone, 2 = smooth (default), higher = steeper sides, flatter top.\n\x09 */" },
#endif
		{ "CPP_Default_FalloffPower", "2.000000" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adds (or subtracts, if negative) a smooth round hill.\n@param Landscape The landscape.\n@param CenterX World X of the hill centre (cm).\n@param CenterY World Y of the hill centre (cm).\n@param RadiusCm Hill radius in cm.\n@param HeightCm Height added at the centre (cm); negative digs.\n@param FalloffPower 1 = linear cone, 2 = smooth (default), higher = steeper sides, flatter top." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function AddHill constinit property declarations *******************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CenterX;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CenterY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_RadiusCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HeightCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_FalloffPower;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function AddHill constinit property declarations *********************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function AddHill Property Definitions ******************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_CenterX = { "CenterX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, CenterX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_CenterY = { "CenterY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, CenterY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_RadiusCm = { "RadiusCm", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, RadiusCm), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_HeightCm = { "HeightCm", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, HeightCm), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_FalloffPower = { "FalloffPower", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddHill_Parms, FalloffPower), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CenterX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CenterY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_RadiusCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HeightCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FalloffPower,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function AddHill Property Definitions ********************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "AddHill", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventAddHill_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventAddHill_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_AddHill(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execAddHill)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FFloatProperty,Z_Param_CenterX);
	P_GET_PROPERTY(FFloatProperty,Z_Param_CenterY);
	P_GET_PROPERTY(FFloatProperty,Z_Param_RadiusCm);
	P_GET_PROPERTY(FFloatProperty,Z_Param_HeightCm);
	P_GET_PROPERTY(FFloatProperty,Z_Param_FalloffPower);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTerrainToolset::AddHill(Z_Param_Landscape,Z_Param_CenterX,Z_Param_CenterY,Z_Param_RadiusCm,Z_Param_HeightCm,Z_Param_FalloffPower);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function AddHill *******************************************

// ********** Begin Class UTerrainToolset Function AddNoise ****************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_AddNoise_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventAddNoise_Parms
	{
		ALandscapeProxy* Landscape;
		float AmplitudeCm;
		float WavelengthCm;
		int32 Octaves;
		int32 Seed;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Adds fractal Perlin noise over the whole landscape.\n\x09 * @param Landscape The landscape.\n\x09 * @param AmplitudeCm Peak amplitude of the first octave (cm); noise is centred on zero.\n\x09 * @param WavelengthCm Wavelength of the first octave (cm).\n\x09 * @param Octaves Number of octaves (1-8); each halves wavelength and amplitude.\n\x09 * @param Seed Offsets the noise field.\n\x09 */" },
#endif
		{ "CPP_Default_Octaves", "4" },
		{ "CPP_Default_Seed", "0" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adds fractal Perlin noise over the whole landscape.\n@param Landscape The landscape.\n@param AmplitudeCm Peak amplitude of the first octave (cm); noise is centred on zero.\n@param WavelengthCm Wavelength of the first octave (cm).\n@param Octaves Number of octaves (1-8); each halves wavelength and amplitude.\n@param Seed Offsets the noise field." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function AddNoise constinit property declarations ******************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_AmplitudeCm;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_WavelengthCm;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Octaves;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Seed;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function AddNoise constinit property declarations ********************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function AddNoise Property Definitions *****************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddNoise_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_AmplitudeCm = { "AmplitudeCm", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddNoise_Parms, AmplitudeCm), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_WavelengthCm = { "WavelengthCm", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddNoise_Parms, WavelengthCm), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_Octaves = { "Octaves", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddNoise_Parms, Octaves), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_Seed = { "Seed", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventAddNoise_Parms, Seed), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_AmplitudeCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WavelengthCm,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Octaves,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Seed,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function AddNoise Property Definitions *******************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "AddNoise", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventAddNoise_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventAddNoise_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_AddNoise(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execAddNoise)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FFloatProperty,Z_Param_AmplitudeCm);
	P_GET_PROPERTY(FFloatProperty,Z_Param_WavelengthCm);
	P_GET_PROPERTY(FIntProperty,Z_Param_Octaves);
	P_GET_PROPERTY(FIntProperty,Z_Param_Seed);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTerrainToolset::AddNoise(Z_Param_Landscape,Z_Param_AmplitudeCm,Z_Param_WavelengthCm,Z_Param_Octaves,Z_Param_Seed);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function AddNoise ******************************************

// ********** Begin Class UTerrainToolset Function CreateLandscape *********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_CreateLandscape_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventCreateLandscape_Parms
	{
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		int32 QuadsPerSection;
		int32 SectionsPerComponent;
		int32 ComponentCountX;
		int32 ComponentCountY;
		UMaterialInterface* Material;
		int32 WorldPartitionGridSize;
		FString HeightmapFile;
		ALandscape* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Creates a new flat landscape in the current level with explicit size parameters.\n\x09 * Total quads per side = ComponentCount * SectionsPerComponent * QuadsPerSection. Vertices per side = quads + 1.\n\x09 * The landscape is centred on Location (same behaviour as the Landscape tool's New Landscape panel).\n\x09 * In a World Partition level the landscape is limited to 16x16 components per call.\n\x09 *\n\x09 * @param Location World location of the landscape centre.\n\x09 * @param Rotation World rotation.\n\x09 * @param Scale Landscape actor scale (X/Y = cm per quad, Z = height scale; 100,100,100 is the engine default).\n\x09 * @param QuadsPerSection Quads per section edge. Must be one of 7, 15, 31, 63, 127, 255.\n\x09 * @param SectionsPerComponent Sections per component edge: 1 (1x1) or 2 (2x2).\n\x09 * @param ComponentCountX Number of components along X.\n\x09 * @param ComponentCountY Number of components along Y.\n\x09 * @param Material Optional landscape material.\n\x09 * @param WorldPartitionGridSize Streaming grid size in components (only used in World Partition levels; engine default 2).\n\x09 * @param HeightmapFile Optional absolute path to a raw 16-bit little-endian heightmap (.r16) with exactly one value per vertex; used as the initial heights (32768 = actor Z).\n\x09 * @return The created landscape actor.\n\x09 */" },
#endif
		{ "CPP_Default_ComponentCountX", "8" },
		{ "CPP_Default_ComponentCountY", "8" },
		{ "CPP_Default_HeightmapFile", "" },
		{ "CPP_Default_Material", "None" },
		{ "CPP_Default_QuadsPerSection", "63" },
		{ "CPP_Default_SectionsPerComponent", "1" },
		{ "CPP_Default_WorldPartitionGridSize", "2" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates a new flat landscape in the current level with explicit size parameters.\nTotal quads per side = ComponentCount * SectionsPerComponent * QuadsPerSection. Vertices per side = quads + 1.\nThe landscape is centred on Location (same behaviour as the Landscape tool's New Landscape panel).\nIn a World Partition level the landscape is limited to 16x16 components per call.\n\n@param Location World location of the landscape centre.\n@param Rotation World rotation.\n@param Scale Landscape actor scale (X/Y = cm per quad, Z = height scale; 100,100,100 is the engine default).\n@param QuadsPerSection Quads per section edge. Must be one of 7, 15, 31, 63, 127, 255.\n@param SectionsPerComponent Sections per component edge: 1 (1x1) or 2 (2x2).\n@param ComponentCountX Number of components along X.\n@param ComponentCountY Number of components along Y.\n@param Material Optional landscape material.\n@param WorldPartitionGridSize Streaming grid size in components (only used in World Partition levels; engine default 2).\n@param HeightmapFile Optional absolute path to a raw 16-bit little-endian heightmap (.r16) with exactly one value per vertex; used as the initial heights (32768 = actor Z).\n@return The created landscape actor." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Location_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Rotation_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Scale_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HeightmapFile_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function CreateLandscape constinit property declarations ***********************
	static const UECodeGen_Private::FStructPropertyParams NewProp_Location;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Rotation;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Scale;
	static const UECodeGen_Private::FIntPropertyParams NewProp_QuadsPerSection;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SectionsPerComponent;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ComponentCountX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ComponentCountY;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Material;
	static const UECodeGen_Private::FIntPropertyParams NewProp_WorldPartitionGridSize;
	static const UECodeGen_Private::FStrPropertyParams NewProp_HeightmapFile;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function CreateLandscape constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function CreateLandscape Property Definitions **********************************
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Location = { "Location", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, Location), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Location_MetaData), NewProp_Location_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Rotation = { "Rotation", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, Rotation), Z_Construct_UScriptStruct_FRotator, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Rotation_MetaData), NewProp_Rotation_MetaData) };
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_Scale = { "Scale", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, Scale), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Scale_MetaData), NewProp_Scale_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_QuadsPerSection = { "QuadsPerSection", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, QuadsPerSection), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SectionsPerComponent = { "SectionsPerComponent", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, SectionsPerComponent), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ComponentCountX = { "ComponentCountX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, ComponentCountX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ComponentCountY = { "ComponentCountY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, ComponentCountY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Material = { "Material", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, Material), Z_Construct_UClass_UMaterialInterface, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_WorldPartitionGridSize = { "WorldPartitionGridSize", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, WorldPartitionGridSize), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_HeightmapFile = { "HeightmapFile", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, HeightmapFile), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HeightmapFile_MetaData), NewProp_HeightmapFile_MetaData) };
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLandscape_Parms, ReturnValue), Z_Construct_UClass_ALandscape, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Location,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Rotation,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Scale,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_QuadsPerSection,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SectionsPerComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ComponentCountX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ComponentCountY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Material,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WorldPartitionGridSize,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HeightmapFile,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function CreateLandscape Property Definitions ************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "CreateLandscape", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventCreateLandscape_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00C22401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventCreateLandscape_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_CreateLandscape(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execCreateLandscape)
{
	P_GET_STRUCT_REF(FVector,Z_Param_Out_Location);
	P_GET_STRUCT_REF(FRotator,Z_Param_Out_Rotation);
	P_GET_STRUCT_REF(FVector,Z_Param_Out_Scale);
	P_GET_PROPERTY(FIntProperty,Z_Param_QuadsPerSection);
	P_GET_PROPERTY(FIntProperty,Z_Param_SectionsPerComponent);
	P_GET_PROPERTY(FIntProperty,Z_Param_ComponentCountX);
	P_GET_PROPERTY(FIntProperty,Z_Param_ComponentCountY);
	P_GET_OBJECT(UMaterialInterface,Z_Param_Material);
	P_GET_PROPERTY(FIntProperty,Z_Param_WorldPartitionGridSize);
	P_GET_PROPERTY(FStrProperty,Z_Param_HeightmapFile);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(ALandscape**)Z_Param__Result=UTerrainToolset::CreateLandscape(Z_Param_Out_Location,Z_Param_Out_Rotation,Z_Param_Out_Scale,Z_Param_QuadsPerSection,Z_Param_SectionsPerComponent,Z_Param_ComponentCountX,Z_Param_ComponentCountY,Z_Param_Material,Z_Param_WorldPartitionGridSize,Z_Param_HeightmapFile);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function CreateLandscape ***********************************

// ********** Begin Class UTerrainToolset Function CreateLayerInfos ********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_CreateLayerInfos_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventCreateLayerInfos_Parms
	{
		ALandscapeProxy* Landscape;
		FString PackagePath;
		TArray<FString> LayerNames;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Creates weight-blended Landscape Layer Info assets (one per name) in a content folder and registers them as\n\x09 * target layers on the landscape so they can be painted. Existing layer infos with the same asset name are reused.\n\x09 * The new assets are left dirty; save them afterwards. Returns a JSON string listing the asset paths.\n\x09 * @param Landscape The landscape.\n\x09 * @param PackagePath Content folder, e.g. /Game/AI_Experiments/Shared/ZoneCoding.\n\x09 * @param LayerNames Layer names exactly as used by the landscape material (e.g. Zone_Path).\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates weight-blended Landscape Layer Info assets (one per name) in a content folder and registers them as\ntarget layers on the landscape so they can be painted. Existing layer infos with the same asset name are reused.\nThe new assets are left dirty; save them afterwards. Returns a JSON string listing the asset paths.\n@param Landscape The landscape.\n@param PackagePath Content folder, e.g. /Game/AI_Experiments/Shared/ZoneCoding.\n@param LayerNames Layer names exactly as used by the landscape material (e.g. Zone_Path)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PackagePath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LayerNames_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function CreateLayerInfos constinit property declarations **********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FStrPropertyParams NewProp_PackagePath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_LayerNames_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_LayerNames;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function CreateLayerInfos constinit property declarations ************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function CreateLayerInfos Property Definitions *********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLayerInfos_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_PackagePath = { "PackagePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLayerInfos_Parms, PackagePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PackagePath_MetaData), NewProp_PackagePath_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_LayerNames_Inner = { "LayerNames", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_LayerNames = { "LayerNames", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLayerInfos_Parms, LayerNames), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LayerNames_MetaData), NewProp_LayerNames_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventCreateLayerInfos_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_PackagePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LayerNames_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LayerNames,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function CreateLayerInfos Property Definitions ***********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "CreateLayerInfos", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventCreateLayerInfos_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00422401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventCreateLayerInfos_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_CreateLayerInfos(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execCreateLayerInfos)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FStrProperty,Z_Param_PackagePath);
	P_GET_TARRAY_REF(FString,Z_Param_Out_LayerNames);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UTerrainToolset::CreateLayerInfos(Z_Param_Landscape,Z_Param_PackagePath,Z_Param_Out_LayerNames);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function CreateLayerInfos **********************************

// ********** Begin Class UTerrainToolset Function GetHeights **************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_GetHeights_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventGetHeights_Parms
	{
		ALandscapeProxy* Landscape;
		int32 MinVertX;
		int32 MinVertY;
		int32 SizeX;
		int32 SizeY;
		TArray<float> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns heights (world Z cm) for a rectangle of vertices, row-major (X varies fastest).\n\x09 * Limited to 65536 vertices per call.\n\x09 * @param Landscape The landscape.\n\x09 * @param MinVertX First vertex X.\n\x09 * @param MinVertY First vertex Y.\n\x09 * @param SizeX Number of vertices along X.\n\x09 * @param SizeY Number of vertices along Y.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns heights (world Z cm) for a rectangle of vertices, row-major (X varies fastest).\nLimited to 65536 vertices per call.\n@param Landscape The landscape.\n@param MinVertX First vertex X.\n@param MinVertY First vertex Y.\n@param SizeX Number of vertices along X.\n@param SizeY Number of vertices along Y." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetHeights constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MinVertX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MinVertY;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SizeX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SizeY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetHeights constinit property declarations ******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetHeights Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_MinVertX = { "MinVertX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, MinVertX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_MinVertY = { "MinVertY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, MinVertY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SizeX = { "SizeX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, SizeX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SizeY = { "SizeY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, SizeY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetHeights_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinVertX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinVertY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SizeX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SizeY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetHeights Property Definitions *****************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "GetHeights", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventGetHeights_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventGetHeights_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_GetHeights(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execGetHeights)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FIntProperty,Z_Param_MinVertX);
	P_GET_PROPERTY(FIntProperty,Z_Param_MinVertY);
	P_GET_PROPERTY(FIntProperty,Z_Param_SizeX);
	P_GET_PROPERTY(FIntProperty,Z_Param_SizeY);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<float>*)Z_Param__Result=UTerrainToolset::GetHeights(Z_Param_Landscape,Z_Param_MinVertX,Z_Param_MinVertY,Z_Param_SizeX,Z_Param_SizeY);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function GetHeights ****************************************

// ********** Begin Class UTerrainToolset Function GetLandscapeInfo ********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_GetLandscapeInfo_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventGetLandscapeInfo_Parms
	{
		ALandscapeProxy* Landscape;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON string describing a landscape: quads per section, sections per component,\n\x09 * quads per component, component counts, total quads/vertices, scale, bounds in cm and metres.\n\x09 * @param Landscape The landscape (or proxy) to inspect.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON string describing a landscape: quads per section, sections per component,\nquads per component, component counts, total quads/vertices, scale, bounds in cm and metres.\n@param Landscape The landscape (or proxy) to inspect." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetLandscapeInfo constinit property declarations **********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetLandscapeInfo constinit property declarations ************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetLandscapeInfo Property Definitions *********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetLandscapeInfo_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventGetLandscapeInfo_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetLandscapeInfo Property Definitions ***********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "GetLandscapeInfo", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventGetLandscapeInfo_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventGetLandscapeInfo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_GetLandscapeInfo(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execGetLandscapeInfo)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UTerrainToolset::GetLandscapeInfo(Z_Param_Landscape);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function GetLandscapeInfo **********************************

// ********** Begin Class UTerrainToolset Function ImportHeightmapFile *****************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_ImportHeightmapFile_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventImportHeightmapFile_Parms
	{
		ALandscapeProxy* Landscape;
		FString FilePath;
		bool bFlipY;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Imports a heightmap file over the whole landscape. Supports .r16/.raw (16-bit little-endian, exact vertex count)\n\x09 * and 16-bit grayscale .png. The file values map straight to the landscape's stored heights (the same as the\n\x09 * Landscape tool's Import), so 32768 = landscape actor Z.\n\x09 * @param Landscape The landscape.\n\x09 * @param FilePath Absolute path to the file.\n\x09 * @param bFlipY Flip the image vertically on import.\n\x09 */" },
#endif
		{ "CPP_Default_bFlipY", "false" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Imports a heightmap file over the whole landscape. Supports .r16/.raw (16-bit little-endian, exact vertex count)\nand 16-bit grayscale .png. The file values map straight to the landscape's stored heights (the same as the\nLandscape tool's Import), so 32768 = landscape actor Z.\n@param Landscape The landscape.\n@param FilePath Absolute path to the file.\n@param bFlipY Flip the image vertically on import." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FilePath_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function ImportHeightmapFile constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FStrPropertyParams NewProp_FilePath;
	static void NewProp_bFlipY_SetBit(void* Obj)
	{
		((TerrainToolset_eventImportHeightmapFile_Parms*)Obj)->bFlipY = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bFlipY;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ImportHeightmapFile constinit property declarations *********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ImportHeightmapFile Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventImportHeightmapFile_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_FilePath = { "FilePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventImportHeightmapFile_Parms, FilePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FilePath_MetaData), NewProp_FilePath_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bFlipY = { "bFlipY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(TerrainToolset_eventImportHeightmapFile_Parms), &UHT_STATICS::NewProp_bFlipY_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FilePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bFlipY,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ImportHeightmapFile Property Definitions ********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "ImportHeightmapFile", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventImportHeightmapFile_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventImportHeightmapFile_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_ImportHeightmapFile(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execImportHeightmapFile)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FStrProperty,Z_Param_FilePath);
	P_GET_UBOOL(Z_Param_bFlipY);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTerrainToolset::ImportHeightmapFile(Z_Param_Landscape,Z_Param_FilePath,Z_Param_bFlipY);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function ImportHeightmapFile *******************************

// ********** Begin Class UTerrainToolset Function LoadWorldRegion *********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_LoadWorldRegion_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventLoadWorldRegion_Parms
	{
		float MinX;
		float MinY;
		float MaxX;
		float MaxY;
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Loads a world-space box of the World Partition editor world so its actors (e.g. landscape proxies) are loaded.\n\x09 * Returns the number of landscape proxies loaded afterwards.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Loads a world-space box of the World Partition editor world so its actors (e.g. landscape proxies) are loaded.\nReturns the number of landscape proxies loaded afterwards." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function LoadWorldRegion constinit property declarations ***********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinX;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MinY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxX;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MaxY;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function LoadWorldRegion constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function LoadWorldRegion Property Definitions **********************************
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MinX = { "MinX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventLoadWorldRegion_Parms, MinX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MinY = { "MinY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventLoadWorldRegion_Parms, MinY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MaxX = { "MaxX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventLoadWorldRegion_Parms, MaxX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_MaxY = { "MaxY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventLoadWorldRegion_Parms, MaxY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventLoadWorldRegion_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function LoadWorldRegion Property Definitions ************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "LoadWorldRegion", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventLoadWorldRegion_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventLoadWorldRegion_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_LoadWorldRegion(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execLoadWorldRegion)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_MinX);
	P_GET_PROPERTY(FFloatProperty,Z_Param_MinY);
	P_GET_PROPERTY(FFloatProperty,Z_Param_MaxX);
	P_GET_PROPERTY(FFloatProperty,Z_Param_MaxY);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=UTerrainToolset::LoadWorldRegion(Z_Param_MinX,Z_Param_MinY,Z_Param_MaxX,Z_Param_MaxY);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function LoadWorldRegion ***********************************

// ********** Begin Class UTerrainToolset Function PaintZoneMap ************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_PaintZoneMap_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventPaintZoneMap_Parms
	{
		ALandscapeProxy* Landscape;
		FString FilePath;
		TArray<FString> LayerNames;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Paints paint layers from a zone index map: a raw 8-bit file with one byte per landscape vertex (row-major, X fastest),\n\x09 * where each byte is an index into LayerNames. Every listed layer gets weight 255 where its index matches and 0 elsewhere,\n\x09 * so each vertex ends up fully on one layer. Layers not in LayerNames are untouched. Layer infos must already exist.\n\x09 * @param Landscape The landscape.\n\x09 * @param FilePath Absolute path to the .r8 index map (exactly one byte per vertex).\n\x09 * @param LayerNames Layer names, indexed by the byte values in the file.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Paints paint layers from a zone index map: a raw 8-bit file with one byte per landscape vertex (row-major, X fastest),\nwhere each byte is an index into LayerNames. Every listed layer gets weight 255 where its index matches and 0 elsewhere,\nso each vertex ends up fully on one layer. Layers not in LayerNames are untouched. Layer infos must already exist.\n@param Landscape The landscape.\n@param FilePath Absolute path to the .r8 index map (exactly one byte per vertex).\n@param LayerNames Layer names, indexed by the byte values in the file." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FilePath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LayerNames_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function PaintZoneMap constinit property declarations **************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FStrPropertyParams NewProp_FilePath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_LayerNames_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_LayerNames;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function PaintZoneMap constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function PaintZoneMap Property Definitions *************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPaintZoneMap_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_FilePath = { "FilePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPaintZoneMap_Parms, FilePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FilePath_MetaData), NewProp_FilePath_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_LayerNames_Inner = { "LayerNames", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_LayerNames = { "LayerNames", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPaintZoneMap_Parms, LayerNames), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LayerNames_MetaData), NewProp_LayerNames_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPaintZoneMap_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FilePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LayerNames_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_LayerNames,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function PaintZoneMap Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "PaintZoneMap", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventPaintZoneMap_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00422401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventPaintZoneMap_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_PaintZoneMap(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execPaintZoneMap)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FStrProperty,Z_Param_FilePath);
	P_GET_TARRAY_REF(FString,Z_Param_Out_LayerNames);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UTerrainToolset::PaintZoneMap(Z_Param_Landscape,Z_Param_FilePath,Z_Param_Out_LayerNames);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function PaintZoneMap **************************************

// ********** Begin Class UTerrainToolset Function PlanLandscape ***********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_PlanLandscape_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventPlanLandscape_Parms
	{
		int32 QuadsPerSection;
		int32 SectionsPerComponent;
		int32 ComponentCountX;
		int32 ComponentCountY;
		float ScaleXY;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON string with the valid parameter values and the size in metres/vertices that a\n\x09 * given parameter combination would produce, for planning before calling CreateLandscape.\n\x09 * @param QuadsPerSection Quads per section edge to evaluate.\n\x09 * @param SectionsPerComponent Sections per component edge to evaluate.\n\x09 * @param ComponentCountX Components along X to evaluate.\n\x09 * @param ComponentCountY Components along Y to evaluate.\n\x09 * @param ScaleXY cm per quad to evaluate.\n\x09 */" },
#endif
		{ "CPP_Default_ComponentCountX", "8" },
		{ "CPP_Default_ComponentCountY", "8" },
		{ "CPP_Default_QuadsPerSection", "63" },
		{ "CPP_Default_ScaleXY", "100.000000" },
		{ "CPP_Default_SectionsPerComponent", "1" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON string with the valid parameter values and the size in metres/vertices that a\ngiven parameter combination would produce, for planning before calling CreateLandscape.\n@param QuadsPerSection Quads per section edge to evaluate.\n@param SectionsPerComponent Sections per component edge to evaluate.\n@param ComponentCountX Components along X to evaluate.\n@param ComponentCountY Components along Y to evaluate.\n@param ScaleXY cm per quad to evaluate." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function PlanLandscape constinit property declarations *************************
	static const UECodeGen_Private::FIntPropertyParams NewProp_QuadsPerSection;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SectionsPerComponent;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ComponentCountX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ComponentCountY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ScaleXY;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function PlanLandscape constinit property declarations ***************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function PlanLandscape Property Definitions ************************************
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_QuadsPerSection = { "QuadsPerSection", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, QuadsPerSection), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SectionsPerComponent = { "SectionsPerComponent", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, SectionsPerComponent), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ComponentCountX = { "ComponentCountX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, ComponentCountX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ComponentCountY = { "ComponentCountY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, ComponentCountY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_ScaleXY = { "ScaleXY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, ScaleXY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventPlanLandscape_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_QuadsPerSection,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SectionsPerComponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ComponentCountX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ComponentCountY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ScaleXY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function PlanLandscape Property Definitions **************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "PlanLandscape", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventPlanLandscape_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventPlanLandscape_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_PlanLandscape(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execPlanLandscape)
{
	P_GET_PROPERTY(FIntProperty,Z_Param_QuadsPerSection);
	P_GET_PROPERTY(FIntProperty,Z_Param_SectionsPerComponent);
	P_GET_PROPERTY(FIntProperty,Z_Param_ComponentCountX);
	P_GET_PROPERTY(FIntProperty,Z_Param_ComponentCountY);
	P_GET_PROPERTY(FFloatProperty,Z_Param_ScaleXY);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UTerrainToolset::PlanLandscape(Z_Param_QuadsPerSection,Z_Param_SectionsPerComponent,Z_Param_ComponentCountX,Z_Param_ComponentCountY,Z_Param_ScaleXY);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function PlanLandscape *************************************

// ********** Begin Class UTerrainToolset Function ReadLayerBlobs **********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_ReadLayerBlobs_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventReadLayerBlobs_Parms
	{
		ALandscapeProxy* Landscape;
		int32 StepVerts;
		int32 Threshold;
		int32 MaxBlobsPerLayer;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Reads the painted weightmaps and returns a JSON string: for every paint layer, its covered area and its\n\x09 * connected regions (blobs) as world-space bounds in metres, centroid and area, largest first.\n\x09 * Sampling is every StepVerts vertices (4 = one sample per 4 m at scale 100).\n\x09 * @param Landscape The landscape.\n\x09 * @param StepVerts Sample spacing in vertices (>=1).\n\x09 * @param Threshold Weight (0-255) at or above which a sample counts as painted.\n\x09 * @param MaxBlobsPerLayer Maximum regions reported per layer.\n\x09 */" },
#endif
		{ "CPP_Default_MaxBlobsPerLayer", "40" },
		{ "CPP_Default_StepVerts", "4" },
		{ "CPP_Default_Threshold", "128" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Reads the painted weightmaps and returns a JSON string: for every paint layer, its covered area and its\nconnected regions (blobs) as world-space bounds in metres, centroid and area, largest first.\nSampling is every StepVerts vertices (4 = one sample per 4 m at scale 100).\n@param Landscape The landscape.\n@param StepVerts Sample spacing in vertices (>=1).\n@param Threshold Weight (0-255) at or above which a sample counts as painted.\n@param MaxBlobsPerLayer Maximum regions reported per layer." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function ReadLayerBlobs constinit property declarations ************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FIntPropertyParams NewProp_StepVerts;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Threshold;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MaxBlobsPerLayer;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ReadLayerBlobs constinit property declarations **************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ReadLayerBlobs Property Definitions ***********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventReadLayerBlobs_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_StepVerts = { "StepVerts", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventReadLayerBlobs_Parms, StepVerts), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_Threshold = { "Threshold", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventReadLayerBlobs_Parms, Threshold), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_MaxBlobsPerLayer = { "MaxBlobsPerLayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventReadLayerBlobs_Parms, MaxBlobsPerLayer), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventReadLayerBlobs_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_StepVerts,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Threshold,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MaxBlobsPerLayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ReadLayerBlobs Property Definitions *************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "ReadLayerBlobs", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventReadLayerBlobs_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventReadLayerBlobs_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_ReadLayerBlobs(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execReadLayerBlobs)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FIntProperty,Z_Param_StepVerts);
	P_GET_PROPERTY(FIntProperty,Z_Param_Threshold);
	P_GET_PROPERTY(FIntProperty,Z_Param_MaxBlobsPerLayer);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UTerrainToolset::ReadLayerBlobs(Z_Param_Landscape,Z_Param_StepVerts,Z_Param_Threshold,Z_Param_MaxBlobsPerLayer);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function ReadLayerBlobs ************************************

// ********** Begin Class UTerrainToolset Function SetFlat *****************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_SetFlat_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventSetFlat_Parms
	{
		ALandscapeProxy* Landscape;
		float HeightCm;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Sets the whole landscape to one constant world height.\n\x09 * @param Landscape The landscape.\n\x09 * @param HeightCm World Z in cm.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets the whole landscape to one constant world height.\n@param Landscape The landscape.\n@param HeightCm World Z in cm." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function SetFlat constinit property declarations *******************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HeightCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetFlat constinit property declarations *********************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetFlat Property Definitions ******************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetFlat_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_HeightCm = { "HeightCm", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetFlat_Parms, HeightCm), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HeightCm,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetFlat Property Definitions ********************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "SetFlat", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventSetFlat_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventSetFlat_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_SetFlat(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execSetFlat)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FFloatProperty,Z_Param_HeightCm);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTerrainToolset::SetFlat(Z_Param_Landscape,Z_Param_HeightCm);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function SetFlat *******************************************

// ********** Begin Class UTerrainToolset Function SetHeights **************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UTerrainToolset_SetHeights_Statics
struct UHT_STATICS
{
	struct TerrainToolset_eventSetHeights_Parms
	{
		ALandscapeProxy* Landscape;
		int32 MinVertX;
		int32 MinVertY;
		int32 SizeX;
		int32 SizeY;
		TArray<float> HeightsCm;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "TerrainToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Writes heights (world Z cm) into a rectangle of vertices, row-major (X varies fastest).\n\x09 * Limited to 65536 vertices per call. Values outside the representable range are clamped.\n\x09 * @param Landscape The landscape.\n\x09 * @param MinVertX First vertex X.\n\x09 * @param MinVertY First vertex Y.\n\x09 * @param SizeX Number of vertices along X.\n\x09 * @param SizeY Number of vertices along Y.\n\x09 * @param HeightsCm SizeX*SizeY world heights in cm.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Writes heights (world Z cm) into a rectangle of vertices, row-major (X varies fastest).\nLimited to 65536 vertices per call. Values outside the representable range are clamped.\n@param Landscape The landscape.\n@param MinVertX First vertex X.\n@param MinVertY First vertex Y.\n@param SizeX Number of vertices along X.\n@param SizeY Number of vertices along Y.\n@param HeightsCm SizeX*SizeY world heights in cm." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HeightsCm_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetHeights constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Landscape;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MinVertX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MinVertY;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SizeX;
	static const UECodeGen_Private::FIntPropertyParams NewProp_SizeY;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HeightsCm_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_HeightsCm;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetHeights constinit property declarations ******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetHeights Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Landscape = { "Landscape", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, Landscape), Z_Construct_UClass_ALandscapeProxy, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_MinVertX = { "MinVertX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, MinVertX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_MinVertY = { "MinVertY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, MinVertY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SizeX = { "SizeX", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, SizeX), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_SizeY = { "SizeY", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, SizeY), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_HeightsCm_Inner = { "HeightsCm", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_HeightsCm = { "HeightsCm", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(TerrainToolset_eventSetHeights_Parms, HeightsCm), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HeightsCm_MetaData), NewProp_HeightsCm_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Landscape,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinVertX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_MinVertY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SizeX,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_SizeY,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HeightsCm_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_HeightsCm,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetHeights Property Definitions *****************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UTerrainToolset, nullptr, "SetHeights", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::TerrainToolset_eventSetHeights_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00422401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::TerrainToolset_eventSetHeights_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UTerrainToolset_SetHeights(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UTerrainToolset::execSetHeights)
{
	P_GET_OBJECT(ALandscapeProxy,Z_Param_Landscape);
	P_GET_PROPERTY(FIntProperty,Z_Param_MinVertX);
	P_GET_PROPERTY(FIntProperty,Z_Param_MinVertY);
	P_GET_PROPERTY(FIntProperty,Z_Param_SizeX);
	P_GET_PROPERTY(FIntProperty,Z_Param_SizeY);
	P_GET_TARRAY_REF(float,Z_Param_Out_HeightsCm);
	P_FINISH;
	P_NATIVE_BEGIN;
	UTerrainToolset::SetHeights(Z_Param_Landscape,Z_Param_MinVertX,Z_Param_MinVertY,Z_Param_SizeX,Z_Param_SizeY,Z_Param_Out_HeightsCm);
	P_NATIVE_END;
}
// ********** End Class UTerrainToolset Function SetHeights ****************************************

// ********** Begin Class UTerrainToolset **********************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UTerrainToolset_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/// Terrain tools: create landscapes from explicit parameters and read back their configuration.\n" },
#endif
		{ "IncludePath", "TerrainToolset.h" },
		{ "ModuleRelativePath", "Public/TerrainToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Terrain tools: create landscapes from explicit parameters and read back their configuration." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UTerrainToolset constinit property declarations **************************
// ********** End Class UTerrainToolset constinit property declarations ****************************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("AddHill"), .Pointer = &UTerrainToolset::execAddHill },
		{ .NameUTF8 = UTF8TEXT("AddNoise"), .Pointer = &UTerrainToolset::execAddNoise },
		{ .NameUTF8 = UTF8TEXT("CreateLandscape"), .Pointer = &UTerrainToolset::execCreateLandscape },
		{ .NameUTF8 = UTF8TEXT("CreateLayerInfos"), .Pointer = &UTerrainToolset::execCreateLayerInfos },
		{ .NameUTF8 = UTF8TEXT("GetHeights"), .Pointer = &UTerrainToolset::execGetHeights },
		{ .NameUTF8 = UTF8TEXT("GetLandscapeInfo"), .Pointer = &UTerrainToolset::execGetLandscapeInfo },
		{ .NameUTF8 = UTF8TEXT("ImportHeightmapFile"), .Pointer = &UTerrainToolset::execImportHeightmapFile },
		{ .NameUTF8 = UTF8TEXT("LoadWorldRegion"), .Pointer = &UTerrainToolset::execLoadWorldRegion },
		{ .NameUTF8 = UTF8TEXT("PaintZoneMap"), .Pointer = &UTerrainToolset::execPaintZoneMap },
		{ .NameUTF8 = UTF8TEXT("PlanLandscape"), .Pointer = &UTerrainToolset::execPlanLandscape },
		{ .NameUTF8 = UTF8TEXT("ReadLayerBlobs"), .Pointer = &UTerrainToolset::execReadLayerBlobs },
		{ .NameUTF8 = UTF8TEXT("SetFlat"), .Pointer = &UTerrainToolset::execSetFlat },
		{ .NameUTF8 = UTF8TEXT("SetHeights"), .Pointer = &UTerrainToolset::execSetHeights },
	};
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UTerrainToolset_AddHill, "AddHill" }, // e720b1178415668be632e1a089e7fa0faaeb9be9
		{ &Z_Construct_UFunction_UTerrainToolset_AddNoise, "AddNoise" }, // eca7768e3bce3af1d6c86fc79c798751c42c766d
		{ &Z_Construct_UFunction_UTerrainToolset_CreateLandscape, "CreateLandscape" }, // 8d8b0976027bb6786c6902a002de8a3c495951d4
		{ &Z_Construct_UFunction_UTerrainToolset_CreateLayerInfos, "CreateLayerInfos" }, // 10899810e3fd498832034abedc20eb7f9ecf2e89
		{ &Z_Construct_UFunction_UTerrainToolset_GetHeights, "GetHeights" }, // c4d757bba7a00ed1f0f10f9c455d519ad1a22ecb
		{ &Z_Construct_UFunction_UTerrainToolset_GetLandscapeInfo, "GetLandscapeInfo" }, // 4c85c79fc530a841c98ff71884e4bd8320aac642
		{ &Z_Construct_UFunction_UTerrainToolset_ImportHeightmapFile, "ImportHeightmapFile" }, // eb429e05b79614ba9aaa6641f93ee42b3f26aa3d
		{ &Z_Construct_UFunction_UTerrainToolset_LoadWorldRegion, "LoadWorldRegion" }, // bcafcee8bffe4cc51f8c9d497294c8c4b42fbb4f
		{ &Z_Construct_UFunction_UTerrainToolset_PaintZoneMap, "PaintZoneMap" }, // 5ee71f35c61fcd03f439b80630c1cb6863c9e62e
		{ &Z_Construct_UFunction_UTerrainToolset_PlanLandscape, "PlanLandscape" }, // 72e91c4f60889fc316a1a55d31b3b1cf339ec899
		{ &Z_Construct_UFunction_UTerrainToolset_ReadLayerBlobs, "ReadLayerBlobs" }, // 6273e3090dfadc5154b81e6107a2656c9aea351a
		{ &Z_Construct_UFunction_UTerrainToolset_SetFlat, "SetFlat" }, // 007caeae44c7196dc383aae979a0b8290ec563f0
		{ &Z_Construct_UFunction_UTerrainToolset_SetHeights, "SetHeights" }, // bcbbb32488e0084526c9524295d6e4686c24dad3
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTerrainToolset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UToolsetDefinition,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_TerrainToolset,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UTerrainToolset,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x000000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static void UTerrainToolset_StaticRegisterNativesUTerrainToolset()
{
	UClass* Class = UTerrainToolset::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, 		MakeConstArrayView(UHT_STATICS::Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTerrainToolset;
UClass* Z_Construct_UClass_UTerrainToolset(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UTerrainToolset;
		if (!Z_Registration_Info_UClass_UTerrainToolset.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("TerrainToolset"),
				Z_Registration_Info_UClass_UTerrainToolset.InnerSingleton,
				UTerrainToolset_StaticRegisterNativesUTerrainToolset,
				DataSizeOf<TClass>(),
				alignof(TClass),
				TClass::StaticClassFlags,
				TClass::StaticClassCastFlags(),
				TClass::StaticConfigName(),
				(UClass::ClassConstructorType)InternalConstructor<TClass>,
				(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
				UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
				&TClass::Super::StaticClass,
				&TClass::WithinClass::StaticClass
			);
		}
		return Z_Registration_Info_UClass_UTerrainToolset.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UTerrainToolset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTerrainToolset.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UTerrainToolset.OuterSingleton;
}
#undef UHT_STATICS
UTerrainToolset::UTerrainToolset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UTerrainToolset);
UTerrainToolset::~UTerrainToolset() {}
// ********** End Class UTerrainToolset ************************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h__Script_TerrainToolset_Statics
struct UHT_STATICS
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTerrainToolset, TEXT("UTerrainToolset"), &Z_Registration_Info_UClass_UTerrainToolset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTerrainToolset), 1119180241U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_TerrainToolset_Source_TerrainToolset_Public_TerrainToolset_h__Script_TerrainToolset_92cdd4f08eb95b61fb4bb5f3b06873c79d019532{
	TEXT("/Script/TerrainToolset"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
