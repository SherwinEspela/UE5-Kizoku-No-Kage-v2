// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "WidgetToolset.h"
#include "UObject/Class.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeWidgetToolset() {}

// ********** Begin Cross Module References ********************************************************
TOOLSETREGISTRY_API UClass* Z_Construct_UClass_UToolsetDefinition(ETypeConstructPhase);
COREUOBJECT_API UClass* Z_Construct_UClass_UClass(ETypeConstructPhase);
NIAGARA_API UClass* Z_Construct_UClass_UNiagaraParameterCollection(ETypeConstructPhase);
UMG_API UClass* Z_Construct_UClass_UUserWidget(ETypeConstructPhase);
UMG_API UClass* Z_Construct_UClass_UWidget(ETypeConstructPhase);
UMGEDITOR_API UClass* Z_Construct_UClass_UWidgetBlueprint(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_WidgetToolset(ETypeConstructPhase);
WIDGETTOOLSET_API UClass* Z_Construct_UClass_UWidgetToolset(ETypeConstructPhase);
WIDGETTOOLSET_API UClass* Z_Construct_UClass_UWidgetToolset(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin Class UWidgetToolset Function AddWidget ****************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_AddWidget_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventAddWidget_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		TSubclassOf<UWidget> WidgetClass;
		FString WidgetName;
		FString ParentName;
		int32 ChildIndex;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Adds a new widget to the tree. ParentName empty makes it the root if the tree is empty. Marks the blueprint\n\x09 * modified (does not save or compile).\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetClass Class of the widget, e.g. /Script/UMG.Overlay or a user widget generated class.\n\x09 * @param WidgetName Desired unique name; empty lets the engine pick one.\n\x09 * @param ParentName Name of the parent panel widget; empty for root.\n\x09 * @param ChildIndex Index in the parent; -1 appends.\n\x09 */" },
#endif
		{ "CPP_Default_ChildIndex", "-1" },
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adds a new widget to the tree. ParentName empty makes it the root if the tree is empty. Marks the blueprint\nmodified (does not save or compile).\n@param WidgetBlueprint The widget blueprint.\n@param WidgetClass Class of the widget, e.g. /Script/UMG.Overlay or a user widget generated class.\n@param WidgetName Desired unique name; empty lets the engine pick one.\n@param ParentName Name of the parent panel widget; empty for root.\n@param ChildIndex Index in the parent; -1 appends." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ParentName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function AddWidget constinit property declarations *****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FClassPropertyParams NewProp_WidgetClass;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ParentName;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ChildIndex;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function AddWidget constinit property declarations *******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function AddWidget Property Definitions ****************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FClassPropertyParams UHT_STATICS::NewProp_WidgetClass = { "WidgetClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, WidgetClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UWidget, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ParentName = { "ParentName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, ParentName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ParentName_MetaData), NewProp_ParentName_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ChildIndex = { "ChildIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, ChildIndex), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventAddWidget_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ParentName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ChildIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function AddWidget Property Definitions ******************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "AddWidget", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventAddWidget_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventAddWidget_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_AddWidget(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execAddWidget)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_OBJECT(UClass,Z_Param_WidgetClass);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_ParentName);
	P_GET_PROPERTY(FIntProperty,Z_Param_ChildIndex);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::AddWidget(Z_Param_WidgetBlueprint,Z_Param_WidgetClass,Z_Param_WidgetName,Z_Param_ParentName,Z_Param_ChildIndex);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function AddWidget ******************************************

// ********** Begin Class UWidgetToolset Function BindWidgetEvent **********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_BindWidgetEvent_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventBindWidgetEvent_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString EventName;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Adds an event node bound to a widget's delegate (like clicking \"+\" next to OnClicked in the Details panel).\n\x09 * The widget must be marked as a variable. Then fill in the graph with the Blueprint tools.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the widget (must be a variable).\n\x09 * @param EventName Delegate property name, e.g. OnClicked, OnHovered.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adds an event node bound to a widget's delegate (like clicking \"+\" next to OnClicked in the Details panel).\nThe widget must be marked as a variable. Then fill in the graph with the Blueprint tools.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the widget (must be a variable).\n@param EventName Delegate property name, e.g. OnClicked, OnHovered." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EventName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function BindWidgetEvent constinit property declarations ***********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_EventName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function BindWidgetEvent constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function BindWidgetEvent Property Definitions **********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventBindWidgetEvent_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventBindWidgetEvent_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_EventName = { "EventName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventBindWidgetEvent_Parms, EventName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EventName_MetaData), NewProp_EventName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventBindWidgetEvent_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_EventName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function BindWidgetEvent Property Definitions ************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "BindWidgetEvent", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventBindWidgetEvent_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventBindWidgetEvent_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_BindWidgetEvent(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execBindWidgetEvent)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_EventName);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::BindWidgetEvent(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_EventName);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function BindWidgetEvent ************************************

// ********** Begin Class UWidgetToolset Function CompileWidgetBlueprint ***************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_CompileWidgetBlueprint_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventCompileWidgetBlueprint_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Compiles the widget blueprint and returns error / warning counts and messages. Does not save.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Compiles the widget blueprint and returns error / warning counts and messages. Does not save.\n@param WidgetBlueprint The widget blueprint." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function CompileWidgetBlueprint constinit property declarations ****************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function CompileWidgetBlueprint constinit property declarations ******************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function CompileWidgetBlueprint Property Definitions ***************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCompileWidgetBlueprint_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCompileWidgetBlueprint_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function CompileWidgetBlueprint Property Definitions *****************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "CompileWidgetBlueprint", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventCompileWidgetBlueprint_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventCompileWidgetBlueprint_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_CompileWidgetBlueprint(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execCompileWidgetBlueprint)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::CompileWidgetBlueprint(Z_Param_WidgetBlueprint);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function CompileWidgetBlueprint *****************************

// ********** Begin Class UWidgetToolset Function CreateParameterCollection ************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_CreateParameterCollection_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventCreateParameterCollection_Parms
	{
		FString FolderPath;
		FString AssetName;
		TArray<FString> FloatParameters;
		TArray<FString> VectorParameters;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Creates a new Niagara Parameter Collection asset with float and vector3 parameters. Blueprint can write these\n\x09 * every tick (Get Niagara Parameter Collection, then Set Float / Vector Parameter) and Niagara systems read\n\x09 * them as NPC_<AssetName>.<Param>. The asset is created in memory (marked dirty); save it with the asset tools.\n\x09 * @param FolderPath Content folder, e.g. /Game/NobleShadows/UI.\n\x09 * @param AssetName Asset name, e.g. NPC_EnemyIndicator. The collection namespace is derived from it.\n\x09 * @param FloatParameters Names of float parameters to add (friendly names, e.g. Proximity01).\n\x09 * @param VectorParameters Names of vector3 parameters to add (friendly names, e.g. LeanDirection).\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates a new Niagara Parameter Collection asset with float and vector3 parameters. Blueprint can write these\nevery tick (Get Niagara Parameter Collection, then Set Float / Vector Parameter) and Niagara systems read\nthem as NPC_<AssetName>.<Param>. The asset is created in memory (marked dirty); save it with the asset tools.\n@param FolderPath Content folder, e.g. /Game/NobleShadows/UI.\n@param AssetName Asset name, e.g. NPC_EnemyIndicator. The collection namespace is derived from it.\n@param FloatParameters Names of float parameters to add (friendly names, e.g. Proximity01).\n@param VectorParameters Names of vector3 parameters to add (friendly names, e.g. LeanDirection)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FolderPath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AssetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FloatParameters_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_VectorParameters_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function CreateParameterCollection constinit property declarations *************
	static const UECodeGen_Private::FStrPropertyParams NewProp_FolderPath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_AssetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_FloatParameters_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_FloatParameters;
	static const UECodeGen_Private::FStrPropertyParams NewProp_VectorParameters_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_VectorParameters;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function CreateParameterCollection constinit property declarations ***************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function CreateParameterCollection Property Definitions ************************
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_FolderPath = { "FolderPath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateParameterCollection_Parms, FolderPath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FolderPath_MetaData), NewProp_FolderPath_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_AssetName = { "AssetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateParameterCollection_Parms, AssetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AssetName_MetaData), NewProp_AssetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_FloatParameters_Inner = { "FloatParameters", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_FloatParameters = { "FloatParameters", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateParameterCollection_Parms, FloatParameters), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FloatParameters_MetaData), NewProp_FloatParameters_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_VectorParameters_Inner = { "VectorParameters", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_VectorParameters = { "VectorParameters", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateParameterCollection_Parms, VectorParameters), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_VectorParameters_MetaData), NewProp_VectorParameters_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateParameterCollection_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FolderPath,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_AssetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FloatParameters_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FloatParameters,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_VectorParameters_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_VectorParameters,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function CreateParameterCollection Property Definitions **************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "CreateParameterCollection", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventCreateParameterCollection_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00422401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventCreateParameterCollection_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_CreateParameterCollection(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execCreateParameterCollection)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_FolderPath);
	P_GET_PROPERTY(FStrProperty,Z_Param_AssetName);
	P_GET_TARRAY_REF(FString,Z_Param_Out_FloatParameters);
	P_GET_TARRAY_REF(FString,Z_Param_Out_VectorParameters);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::CreateParameterCollection(Z_Param_FolderPath,Z_Param_AssetName,Z_Param_Out_FloatParameters,Z_Param_Out_VectorParameters);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function CreateParameterCollection **************************

// ********** Begin Class UWidgetToolset Function CreateWidgetBlueprint ****************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_CreateWidgetBlueprint_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventCreateWidgetBlueprint_Parms
	{
		FString FolderPath;
		FString AssetName;
		TSubclassOf<UUserWidget> ParentClass;
		TSubclassOf<UWidget> RootWidgetClass;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Sets the default value of a float or vector3 parameter on a collection (the value Niagara uses until Blueprint\n\x09 * overrides it). For a vector pass X, Y, Z; for a float only X is used.\n\x09 * @param Collection The Niagara Parameter Collection asset.\n\x09 * @param ParameterName Friendly parameter name.\n\x09 * @param X Float value, or vector X.\n\x09 * @param Y Vector Y (ignored for floats).\n\x09 * @param Z Vector Z (ignored for floats).\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets the default value of a float or vector3 parameter on a collection (the value Niagara uses until Blueprint\noverrides it). For a vector pass X, Y, Z; for a float only X is used.\n@param Collection The Niagara Parameter Collection asset.\n@param ParameterName Friendly parameter name.\n@param X Float value, or vector X.\n@param Y Vector Y (ignored for floats).\n@param Z Vector Z (ignored for floats)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FolderPath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AssetName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function CreateWidgetBlueprint constinit property declarations *****************
	static const UECodeGen_Private::FStrPropertyParams NewProp_FolderPath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_AssetName;
	static const UECodeGen_Private::FClassPropertyParams NewProp_ParentClass;
	static const UECodeGen_Private::FClassPropertyParams NewProp_RootWidgetClass;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function CreateWidgetBlueprint constinit property declarations *******************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function CreateWidgetBlueprint Property Definitions ****************************
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_FolderPath = { "FolderPath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateWidgetBlueprint_Parms, FolderPath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FolderPath_MetaData), NewProp_FolderPath_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_AssetName = { "AssetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateWidgetBlueprint_Parms, AssetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AssetName_MetaData), NewProp_AssetName_MetaData) };
const UECodeGen_Private::FClassPropertyParams UHT_STATICS::NewProp_ParentClass = { "ParentClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateWidgetBlueprint_Parms, ParentClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UUserWidget, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FClassPropertyParams UHT_STATICS::NewProp_RootWidgetClass = { "RootWidgetClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateWidgetBlueprint_Parms, RootWidgetClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UWidget, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventCreateWidgetBlueprint_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_FolderPath,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_AssetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ParentClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_RootWidgetClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function CreateWidgetBlueprint Property Definitions ******************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "CreateWidgetBlueprint", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventCreateWidgetBlueprint_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventCreateWidgetBlueprint_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_CreateWidgetBlueprint(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execCreateWidgetBlueprint)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_FolderPath);
	P_GET_PROPERTY(FStrProperty,Z_Param_AssetName);
	P_GET_OBJECT(UClass,Z_Param_ParentClass);
	P_GET_OBJECT(UClass,Z_Param_RootWidgetClass);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::CreateWidgetBlueprint(Z_Param_FolderPath,Z_Param_AssetName,Z_Param_ParentClass,Z_Param_RootWidgetClass);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function CreateWidgetBlueprint ******************************

// ********** Begin Class UWidgetToolset Function FindWidgetClasses ********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_FindWidgetClasses_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventFindWidgetClasses_Parms
	{
		FString NameFilter;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Lists loaded, non-abstract UWidget subclasses (native and loaded blueprint classes) whose name contains the\n\x09 * filter, with the class path to pass to AddWidget. Widget blueprints that are not loaded are not listed; for\n\x09 * those pass the generated class path (e.g. /Game/.../WBP_Foo.WBP_Foo_C) directly to AddWidget.\n\x09 * @param NameFilter Case-insensitive substring of the class name; empty returns all.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Lists loaded, non-abstract UWidget subclasses (native and loaded blueprint classes) whose name contains the\nfilter, with the class path to pass to AddWidget. Widget blueprints that are not loaded are not listed; for\nthose pass the generated class path (e.g. /Game/.../WBP_Foo.WBP_Foo_C) directly to AddWidget.\n@param NameFilter Case-insensitive substring of the class name; empty returns all." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NameFilter_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function FindWidgetClasses constinit property declarations *********************
	static const UECodeGen_Private::FStrPropertyParams NewProp_NameFilter;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function FindWidgetClasses constinit property declarations ***********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function FindWidgetClasses Property Definitions ********************************
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_NameFilter = { "NameFilter", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventFindWidgetClasses_Parms, NameFilter), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NameFilter_MetaData), NewProp_NameFilter_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventFindWidgetClasses_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NameFilter,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function FindWidgetClasses Property Definitions **********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "FindWidgetClasses", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventFindWidgetClasses_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventFindWidgetClasses_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_FindWidgetClasses(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execFindWidgetClasses)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_NameFilter);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::FindWidgetClasses(Z_Param_NameFilter);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function FindWidgetClasses **********************************

// ********** Begin Class UWidgetToolset Function GetParameterCollectionInfo ***********************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_GetParameterCollectionInfo_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventGetParameterCollectionInfo_Parms
	{
		UNiagaraParameterCollection* Collection;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Lists a parameter collection's namespace and parameters (friendly name, type, current default value).\n\x09 * @param Collection The Niagara Parameter Collection asset.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Lists a parameter collection's namespace and parameters (friendly name, type, current default value).\n@param Collection The Niagara Parameter Collection asset." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetParameterCollectionInfo constinit property declarations ************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Collection;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetParameterCollectionInfo constinit property declarations **************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetParameterCollectionInfo Property Definitions ***********************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Collection = { "Collection", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetParameterCollectionInfo_Parms, Collection), Z_Construct_UClass_UNiagaraParameterCollection, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetParameterCollectionInfo_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Collection,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetParameterCollectionInfo Property Definitions *************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "GetParameterCollectionInfo", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventGetParameterCollectionInfo_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventGetParameterCollectionInfo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_GetParameterCollectionInfo(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execGetParameterCollectionInfo)
{
	P_GET_OBJECT(UNiagaraParameterCollection,Z_Param_Collection);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::GetParameterCollectionInfo(Z_Param_Collection);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function GetParameterCollectionInfo *************************

// ********** Begin Class UWidgetToolset Function GetWidgetProperties ******************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_GetWidgetProperties_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventGetWidgetProperties_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		bool bAllProperties;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns the editable properties of one widget and of its panel slot as exported text. By default only\n\x09 * properties that differ from the class default are returned.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the widget in the tree.\n\x09 * @param bAllProperties True to return every editable property, not only non-default ones.\n\x09 */" },
#endif
		{ "CPP_Default_bAllProperties", "false" },
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the editable properties of one widget and of its panel slot as exported text. By default only\nproperties that differ from the class default are returned.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the widget in the tree.\n@param bAllProperties True to return every editable property, not only non-default ones." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function GetWidgetProperties constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static void NewProp_bAllProperties_SetBit(void* Obj)
	{
		((WidgetToolset_eventGetWidgetProperties_Parms*)Obj)->bAllProperties = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bAllProperties;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetWidgetProperties constinit property declarations *********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetWidgetProperties Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetWidgetProperties_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetWidgetProperties_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bAllProperties = { "bAllProperties", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(WidgetToolset_eventGetWidgetProperties_Parms), &UHT_STATICS::NewProp_bAllProperties_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetWidgetProperties_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bAllProperties,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetWidgetProperties Property Definitions ********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "GetWidgetProperties", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventGetWidgetProperties_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventGetWidgetProperties_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_GetWidgetProperties(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execGetWidgetProperties)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_UBOOL(Z_Param_bAllProperties);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::GetWidgetProperties(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_bAllProperties);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function GetWidgetProperties ********************************

// ********** Begin Class UWidgetToolset Function GetWidgetTree ************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_GetWidgetTree_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventGetWidgetTree_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns the widget tree of a widget blueprint as nested JSON: per widget its name, class path, bIsVariable,\n\x09 * visibility, slot class, panel children (\"children\") and named-slot content (\"namedSlots\"). Also lists the\n\x09 * parent class and the animation names. Does not descend into nested user widgets.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the widget tree of a widget blueprint as nested JSON: per widget its name, class path, bIsVariable,\nvisibility, slot class, panel children (\"children\") and named-slot content (\"namedSlots\"). Also lists the\nparent class and the animation names. Does not descend into nested user widgets.\n@param WidgetBlueprint The widget blueprint." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetWidgetTree constinit property declarations *************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetWidgetTree constinit property declarations ***************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetWidgetTree Property Definitions ************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetWidgetTree_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventGetWidgetTree_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetWidgetTree Property Definitions **************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "GetWidgetTree", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventGetWidgetTree_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventGetWidgetTree_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_GetWidgetTree(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execGetWidgetTree)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::GetWidgetTree(Z_Param_WidgetBlueprint);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function GetWidgetTree **************************************

// ********** Begin Class UWidgetToolset Function ListWidgetAnimations *****************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_ListWidgetAnimations_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventListWidgetAnimations_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Lists the widget animations of a blueprint: name, start/end time in seconds, the widgets they are bound to\n\x09 * and the track class / display name for every binding. Read-only.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Lists the widget animations of a blueprint: name, start/end time in seconds, the widgets they are bound to\nand the track class / display name for every binding. Read-only.\n@param WidgetBlueprint The widget blueprint." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function ListWidgetAnimations constinit property declarations ******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ListWidgetAnimations constinit property declarations ********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ListWidgetAnimations Property Definitions *****************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventListWidgetAnimations_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventListWidgetAnimations_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ListWidgetAnimations Property Definitions *******************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "ListWidgetAnimations", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventListWidgetAnimations_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventListWidgetAnimations_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_ListWidgetAnimations(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execListWidgetAnimations)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::ListWidgetAnimations(Z_Param_WidgetBlueprint);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function ListWidgetAnimations *******************************

// ********** Begin Class UWidgetToolset Function MoveWidget ***************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_MoveWidget_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventMoveWidget_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString NewParentName;
		int32 ChildIndex;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Moves a widget to a new parent panel at an index. Marks the blueprint modified.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Widget to move.\n\x09 * @param NewParentName Name of the new parent panel widget.\n\x09 * @param ChildIndex Index in the new parent; -1 appends.\n\x09 */" },
#endif
		{ "CPP_Default_ChildIndex", "-1" },
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Moves a widget to a new parent panel at an index. Marks the blueprint modified.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Widget to move.\n@param NewParentName Name of the new parent panel widget.\n@param ChildIndex Index in the new parent; -1 appends." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewParentName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function MoveWidget constinit property declarations ****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_NewParentName;
	static const UECodeGen_Private::FIntPropertyParams NewProp_ChildIndex;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function MoveWidget constinit property declarations ******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function MoveWidget Property Definitions ***************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventMoveWidget_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventMoveWidget_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_NewParentName = { "NewParentName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventMoveWidget_Parms, NewParentName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewParentName_MetaData), NewProp_NewParentName_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_ChildIndex = { "ChildIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventMoveWidget_Parms, ChildIndex), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventMoveWidget_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NewParentName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ChildIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function MoveWidget Property Definitions *****************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "MoveWidget", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventMoveWidget_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventMoveWidget_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_MoveWidget(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execMoveWidget)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_NewParentName);
	P_GET_PROPERTY(FIntProperty,Z_Param_ChildIndex);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::MoveWidget(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_NewParentName,Z_Param_ChildIndex);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function MoveWidget *****************************************

// ********** Begin Class UWidgetToolset Function RemoveWidget *************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_RemoveWidget_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventRemoveWidget_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Removes a widget and all of its children from the tree (cleans references). Marks the blueprint modified.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the widget to remove.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Removes a widget and all of its children from the tree (cleans references). Marks the blueprint modified.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the widget to remove." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function RemoveWidget constinit property declarations **************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function RemoveWidget constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function RemoveWidget Property Definitions *************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRemoveWidget_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRemoveWidget_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRemoveWidget_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function RemoveWidget Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "RemoveWidget", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventRemoveWidget_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventRemoveWidget_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_RemoveWidget(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execRemoveWidget)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::RemoveWidget(Z_Param_WidgetBlueprint,Z_Param_WidgetName);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function RemoveWidget ***************************************

// ********** Begin Class UWidgetToolset Function RenameWidget *************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_RenameWidget_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventRenameWidget_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString NewName;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Renames a widget (also renames its variable and updates references). Marks the blueprint modified.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Current name.\n\x09 * @param NewName New name; must be unique and a valid variable name.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Renames a widget (also renames its variable and updates references). Marks the blueprint modified.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Current name.\n@param NewName New name; must be unique and a valid variable name." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_NewName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function RenameWidget constinit property declarations **************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_NewName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function RenameWidget constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function RenameWidget Property Definitions *************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRenameWidget_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRenameWidget_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_NewName = { "NewName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRenameWidget_Parms, NewName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_NewName_MetaData), NewProp_NewName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventRenameWidget_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NewName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function RenameWidget Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "RenameWidget", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventRenameWidget_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventRenameWidget_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_RenameWidget(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execRenameWidget)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_NewName);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::RenameWidget(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_NewName);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function RenameWidget ***************************************

// ********** Begin Class UWidgetToolset Function SetParameterCollectionDefault ********************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_SetParameterCollectionDefault_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventSetParameterCollectionDefault_Parms
	{
		UNiagaraParameterCollection* Collection;
		FString ParameterName;
		float X;
		float Y;
		float Z;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
		{ "CPP_Default_Y", "0.000000" },
		{ "CPP_Default_Z", "0.000000" },
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ParameterName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetParameterCollectionDefault constinit property declarations *********
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Collection;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ParameterName;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_X;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Y;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Z;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetParameterCollectionDefault constinit property declarations ***********
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetParameterCollectionDefault Property Definitions ********************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Collection = { "Collection", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, Collection), Z_Construct_UClass_UNiagaraParameterCollection, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ParameterName = { "ParameterName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, ParameterName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ParameterName_MetaData), NewProp_ParameterName_MetaData) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_X = { "X", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, X), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Y = { "Y", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, Y), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_Z = { "Z", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, Z), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetParameterCollectionDefault_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Collection,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ParameterName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_X,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Y,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Z,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetParameterCollectionDefault Property Definitions **********************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "SetParameterCollectionDefault", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventSetParameterCollectionDefault_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventSetParameterCollectionDefault_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_SetParameterCollectionDefault(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execSetParameterCollectionDefault)
{
	P_GET_OBJECT(UNiagaraParameterCollection,Z_Param_Collection);
	P_GET_PROPERTY(FStrProperty,Z_Param_ParameterName);
	P_GET_PROPERTY(FFloatProperty,Z_Param_X);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Y);
	P_GET_PROPERTY(FFloatProperty,Z_Param_Z);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::SetParameterCollectionDefault(Z_Param_Collection,Z_Param_ParameterName,Z_Param_X,Z_Param_Y,Z_Param_Z);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function SetParameterCollectionDefault **********************

// ********** Begin Class UWidgetToolset Function SetSlotProperty **********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_SetSlotProperty_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventSetSlotProperty_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString PropertyName;
		FString ValueText;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Sets one editable property on the widget's panel slot (padding, anchors, alignment, size...) from exported text.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the child widget whose slot to edit.\n\x09 * @param PropertyName Slot property, e.g. Padding, HorizontalAlignment, LayoutData.\n\x09 * @param ValueText Value in Unreal text format.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets one editable property on the widget's panel slot (padding, anchors, alignment, size...) from exported text.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the child widget whose slot to edit.\n@param PropertyName Slot property, e.g. Padding, HorizontalAlignment, LayoutData.\n@param ValueText Value in Unreal text format." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PropertyName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ValueText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetSlotProperty constinit property declarations ***********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_PropertyName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ValueText;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetSlotProperty constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetSlotProperty Property Definitions **********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetSlotProperty_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetSlotProperty_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_PropertyName = { "PropertyName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetSlotProperty_Parms, PropertyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PropertyName_MetaData), NewProp_PropertyName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ValueText = { "ValueText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetSlotProperty_Parms, ValueText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ValueText_MetaData), NewProp_ValueText_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetSlotProperty_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_PropertyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ValueText,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetSlotProperty Property Definitions ************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "SetSlotProperty", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventSetSlotProperty_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventSetSlotProperty_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_SetSlotProperty(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execSetSlotProperty)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_PropertyName);
	P_GET_PROPERTY(FStrProperty,Z_Param_ValueText);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::SetSlotProperty(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_PropertyName,Z_Param_ValueText);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function SetSlotProperty ************************************

// ********** Begin Class UWidgetToolset Function SetWidgetIsVariable ******************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_SetWidgetIsVariable_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventSetWidgetIsVariable_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		bool bIsVariable;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Sets or clears the \"Is Variable\" flag of a widget so it can be referenced in graphs. Marks the blueprint modified.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the widget.\n\x09 * @param bIsVariable The new value.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets or clears the \"Is Variable\" flag of a widget so it can be referenced in graphs. Marks the blueprint modified.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the widget.\n@param bIsVariable The new value." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetWidgetIsVariable constinit property declarations *******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static void NewProp_bIsVariable_SetBit(void* Obj)
	{
		((WidgetToolset_eventSetWidgetIsVariable_Parms*)Obj)->bIsVariable = 1;
	}
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsVariable;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetWidgetIsVariable constinit property declarations *********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetWidgetIsVariable Property Definitions ******************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetIsVariable_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetIsVariable_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FBoolPropertyParams UHT_STATICS::NewProp_bIsVariable = { "bIsVariable", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, nullptr, nullptr, 1, sizeof(bool), sizeof(WidgetToolset_eventSetWidgetIsVariable_Parms), &UHT_STATICS::NewProp_bIsVariable_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetIsVariable_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_bIsVariable,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetWidgetIsVariable Property Definitions ********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "SetWidgetIsVariable", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventSetWidgetIsVariable_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventSetWidgetIsVariable_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_SetWidgetIsVariable(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execSetWidgetIsVariable)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_UBOOL(Z_Param_bIsVariable);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::SetWidgetIsVariable(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_bIsVariable);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function SetWidgetIsVariable ********************************

// ********** Begin Class UWidgetToolset Function SetWidgetProperty ********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UWidgetToolset_SetWidgetProperty_Statics
struct UHT_STATICS
{
	struct WidgetToolset_eventSetWidgetProperty_Parms
	{
		UWidgetBlueprint* WidgetBlueprint;
		FString WidgetName;
		FString PropertyName;
		FString ValueText;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "WidgetToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Sets one editable property on a widget from exported text (same format GetWidgetProperties returns), e.g.\n\x09 * Visibility = Collapsed, or a struct as \"(R=1,G=0,B=0,A=1)\". Returns the property value after the change.\n\x09 * @param WidgetBlueprint The widget blueprint.\n\x09 * @param WidgetName Name of the widget.\n\x09 * @param PropertyName Property name, e.g. Visibility, Text, RenderOpacity.\n\x09 * @param ValueText Value in Unreal text format.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets one editable property on a widget from exported text (same format GetWidgetProperties returns), e.g.\nVisibility = Collapsed, or a struct as \"(R=1,G=0,B=0,A=1)\". Returns the property value after the change.\n@param WidgetBlueprint The widget blueprint.\n@param WidgetName Name of the widget.\n@param PropertyName Property name, e.g. Visibility, Text, RenderOpacity.\n@param ValueText Value in Unreal text format." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WidgetName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PropertyName_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ValueText_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetWidgetProperty constinit property declarations *********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetBlueprint;
	static const UECodeGen_Private::FStrPropertyParams NewProp_WidgetName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_PropertyName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ValueText;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetWidgetProperty constinit property declarations ***********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetWidgetProperty Property Definitions ********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_WidgetBlueprint = { "WidgetBlueprint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetProperty_Parms, WidgetBlueprint), Z_Construct_UClass_UWidgetBlueprint, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_WidgetName = { "WidgetName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetProperty_Parms, WidgetName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WidgetName_MetaData), NewProp_WidgetName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_PropertyName = { "PropertyName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetProperty_Parms, PropertyName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PropertyName_MetaData), NewProp_PropertyName_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ValueText = { "ValueText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetProperty_Parms, ValueText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ValueText_MetaData), NewProp_ValueText_MetaData) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(WidgetToolset_eventSetWidgetProperty_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetBlueprint,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_WidgetName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_PropertyName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ValueText,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function SetWidgetProperty Property Definitions **********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UWidgetToolset, nullptr, "SetWidgetProperty", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::WidgetToolset_eventSetWidgetProperty_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::WidgetToolset_eventSetWidgetProperty_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UWidgetToolset_SetWidgetProperty(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UWidgetToolset::execSetWidgetProperty)
{
	P_GET_OBJECT(UWidgetBlueprint,Z_Param_WidgetBlueprint);
	P_GET_PROPERTY(FStrProperty,Z_Param_WidgetName);
	P_GET_PROPERTY(FStrProperty,Z_Param_PropertyName);
	P_GET_PROPERTY(FStrProperty,Z_Param_ValueText);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UWidgetToolset::SetWidgetProperty(Z_Param_WidgetBlueprint,Z_Param_WidgetName,Z_Param_PropertyName,Z_Param_ValueText);
	P_NATIVE_END;
}
// ********** End Class UWidgetToolset Function SetWidgetProperty **********************************

// ********** Begin Class UWidgetToolset ***********************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UWidgetToolset_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * UMG / CommonUI widget blueprint tools. Read tools first, then write tools. Write tools mark the blueprint\n * modified but never save; call CompileWidgetBlueprint and then save with the asset tools.\n * All results are JSON strings: {\"ok\":true,...} or {\"ok\":false,\"error\":\"...\"}.\n */" },
#endif
		{ "IncludePath", "WidgetToolset.h" },
		{ "ModuleRelativePath", "Public/WidgetToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UMG / CommonUI widget blueprint tools. Read tools first, then write tools. Write tools mark the blueprint\nmodified but never save; call CompileWidgetBlueprint and then save with the asset tools.\nAll results are JSON strings: {\"ok\":true,...} or {\"ok\":false,\"error\":\"...\"}." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UWidgetToolset constinit property declarations ***************************
// ********** End Class UWidgetToolset constinit property declarations *****************************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("AddWidget"), .Pointer = &UWidgetToolset::execAddWidget },
		{ .NameUTF8 = UTF8TEXT("BindWidgetEvent"), .Pointer = &UWidgetToolset::execBindWidgetEvent },
		{ .NameUTF8 = UTF8TEXT("CompileWidgetBlueprint"), .Pointer = &UWidgetToolset::execCompileWidgetBlueprint },
		{ .NameUTF8 = UTF8TEXT("CreateParameterCollection"), .Pointer = &UWidgetToolset::execCreateParameterCollection },
		{ .NameUTF8 = UTF8TEXT("CreateWidgetBlueprint"), .Pointer = &UWidgetToolset::execCreateWidgetBlueprint },
		{ .NameUTF8 = UTF8TEXT("FindWidgetClasses"), .Pointer = &UWidgetToolset::execFindWidgetClasses },
		{ .NameUTF8 = UTF8TEXT("GetParameterCollectionInfo"), .Pointer = &UWidgetToolset::execGetParameterCollectionInfo },
		{ .NameUTF8 = UTF8TEXT("GetWidgetProperties"), .Pointer = &UWidgetToolset::execGetWidgetProperties },
		{ .NameUTF8 = UTF8TEXT("GetWidgetTree"), .Pointer = &UWidgetToolset::execGetWidgetTree },
		{ .NameUTF8 = UTF8TEXT("ListWidgetAnimations"), .Pointer = &UWidgetToolset::execListWidgetAnimations },
		{ .NameUTF8 = UTF8TEXT("MoveWidget"), .Pointer = &UWidgetToolset::execMoveWidget },
		{ .NameUTF8 = UTF8TEXT("RemoveWidget"), .Pointer = &UWidgetToolset::execRemoveWidget },
		{ .NameUTF8 = UTF8TEXT("RenameWidget"), .Pointer = &UWidgetToolset::execRenameWidget },
		{ .NameUTF8 = UTF8TEXT("SetParameterCollectionDefault"), .Pointer = &UWidgetToolset::execSetParameterCollectionDefault },
		{ .NameUTF8 = UTF8TEXT("SetSlotProperty"), .Pointer = &UWidgetToolset::execSetSlotProperty },
		{ .NameUTF8 = UTF8TEXT("SetWidgetIsVariable"), .Pointer = &UWidgetToolset::execSetWidgetIsVariable },
		{ .NameUTF8 = UTF8TEXT("SetWidgetProperty"), .Pointer = &UWidgetToolset::execSetWidgetProperty },
	};
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UWidgetToolset_AddWidget, "AddWidget" }, // a89763da185142ab668352f3c2a229517632f808
		{ &Z_Construct_UFunction_UWidgetToolset_BindWidgetEvent, "BindWidgetEvent" }, // 03b43c25f93aa09c34946a8faef758e368d7cb74
		{ &Z_Construct_UFunction_UWidgetToolset_CompileWidgetBlueprint, "CompileWidgetBlueprint" }, // 1feede55d2e2636d09ac10489dface78c67b192a
		{ &Z_Construct_UFunction_UWidgetToolset_CreateParameterCollection, "CreateParameterCollection" }, // 49d8f5bca7744fa2aaf8a1e9eac96fe1c04941d5
		{ &Z_Construct_UFunction_UWidgetToolset_CreateWidgetBlueprint, "CreateWidgetBlueprint" }, // c41776c2e3ee838c36bac00ed79d146c1a8e4429
		{ &Z_Construct_UFunction_UWidgetToolset_FindWidgetClasses, "FindWidgetClasses" }, // 128de9aad72060dc80dd2a082c199d62ea800454
		{ &Z_Construct_UFunction_UWidgetToolset_GetParameterCollectionInfo, "GetParameterCollectionInfo" }, // 92053a1403d06a8359de1362135c8f2900702cc2
		{ &Z_Construct_UFunction_UWidgetToolset_GetWidgetProperties, "GetWidgetProperties" }, // b60efc359d195c288df3f935778b5d38c613830e
		{ &Z_Construct_UFunction_UWidgetToolset_GetWidgetTree, "GetWidgetTree" }, // f3988a1935cd096ac92911f4e416eae98149efc5
		{ &Z_Construct_UFunction_UWidgetToolset_ListWidgetAnimations, "ListWidgetAnimations" }, // 4b5199aa93ed2f8e7328496bb68f67ea297b213f
		{ &Z_Construct_UFunction_UWidgetToolset_MoveWidget, "MoveWidget" }, // 2a390bbfc97b4cc8513e6cd70277aca41f330c2f
		{ &Z_Construct_UFunction_UWidgetToolset_RemoveWidget, "RemoveWidget" }, // 482da585904df2df7cba4240646ef28d0cc5f748
		{ &Z_Construct_UFunction_UWidgetToolset_RenameWidget, "RenameWidget" }, // 2365a60077fd8b510f244e9979aac4044bc41f40
		{ &Z_Construct_UFunction_UWidgetToolset_SetParameterCollectionDefault, "SetParameterCollectionDefault" }, // d4d1f4d22b36306fe433d5da35d387d22df939d7
		{ &Z_Construct_UFunction_UWidgetToolset_SetSlotProperty, "SetSlotProperty" }, // 5e77208809f9ab87865f253df9d8bc863aec2f0c
		{ &Z_Construct_UFunction_UWidgetToolset_SetWidgetIsVariable, "SetWidgetIsVariable" }, // 3bb47dd2a28ce205aea70852322bd619fdfc357f
		{ &Z_Construct_UFunction_UWidgetToolset_SetWidgetProperty, "SetWidgetProperty" }, // f18d2673b8a16df2d5073508ac9380a0fb34903e
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UWidgetToolset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UToolsetDefinition,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_WidgetToolset,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UWidgetToolset,
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
static void UWidgetToolset_StaticRegisterNativesUWidgetToolset()
{
	UClass* Class = UWidgetToolset::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, 		MakeConstArrayView(UHT_STATICS::Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UWidgetToolset;
UClass* Z_Construct_UClass_UWidgetToolset(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UWidgetToolset;
		if (!Z_Registration_Info_UClass_UWidgetToolset.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("WidgetToolset"),
				Z_Registration_Info_UClass_UWidgetToolset.InnerSingleton,
				UWidgetToolset_StaticRegisterNativesUWidgetToolset,
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
		return Z_Registration_Info_UClass_UWidgetToolset.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UWidgetToolset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UWidgetToolset.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UWidgetToolset.OuterSingleton;
}
#undef UHT_STATICS
UWidgetToolset::UWidgetToolset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UWidgetToolset);
UWidgetToolset::~UWidgetToolset() {}
// ********** End Class UWidgetToolset *************************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h__Script_WidgetToolset_Statics
struct UHT_STATICS
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UWidgetToolset, TEXT("UWidgetToolset"), &Z_Registration_Info_UClass_UWidgetToolset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UWidgetToolset), 1969145241U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h__Script_WidgetToolset_70adf77d409a4dd189b970f610a6ec5fefe49b19{
	TEXT("/Script/WidgetToolset"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
