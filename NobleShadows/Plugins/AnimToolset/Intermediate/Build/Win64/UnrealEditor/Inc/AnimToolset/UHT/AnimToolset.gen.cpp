// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "AnimToolset.h"
#include "UObject/Class.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeAnimToolset() {}

// ********** Begin Cross Module References ********************************************************
TOOLSETREGISTRY_API UClass* Z_Construct_UClass_UToolsetDefinition(ETypeConstructPhase);
COREUOBJECT_API UClass* Z_Construct_UClass_UClass(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UAnimNotify(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UAnimSequence(ETypeConstructPhase);
ENGINE_API UClass* Z_Construct_UClass_UAnimSequenceBase(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_AnimToolset(ETypeConstructPhase);
ANIMTOOLSET_API UClass* Z_Construct_UClass_UAnimToolset(ETypeConstructPhase);
ANIMTOOLSET_API UClass* Z_Construct_UClass_UAnimToolset(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin Class UAnimToolset Function AddNotify ******************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_AddNotify_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventAddNotify_Parms
	{
		UAnimSequenceBase* Sequence;
		TSubclassOf<UAnimNotify> NotifyClass;
		float TimeSeconds;
		int32 TrackIndex;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Adds an instanced notify of the given class to a sequence at a time in seconds. Marks the asset dirty (does not save).\n\x09 * Returns JSON with the new notify index. Does not add if a notify of the same class already exists within 1 ms.\n\x09 * @param Sequence The animation sequence.\n\x09 * @param NotifyClass Notify class, e.g. a Blueprint AnimNotify generated class (..._C).\n\x09 * @param TimeSeconds Time of the notify in seconds, within the sequence length.\n\x09 * @param TrackIndex Notify track to place it on (0 based).\n\x09 */" },
#endif
		{ "CPP_Default_TrackIndex", "0" },
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Adds an instanced notify of the given class to a sequence at a time in seconds. Marks the asset dirty (does not save).\nReturns JSON with the new notify index. Does not add if a notify of the same class already exists within 1 ms.\n@param Sequence The animation sequence.\n@param NotifyClass Notify class, e.g. a Blueprint AnimNotify generated class (..._C).\n@param TimeSeconds Time of the notify in seconds, within the sequence length.\n@param TrackIndex Notify track to place it on (0 based)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function AddNotify constinit property declarations *****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FClassPropertyParams NewProp_NotifyClass;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_TimeSeconds;
	static const UECodeGen_Private::FIntPropertyParams NewProp_TrackIndex;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function AddNotify constinit property declarations *******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function AddNotify Property Definitions ****************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventAddNotify_Parms, Sequence), Z_Construct_UClass_UAnimSequenceBase, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FClassPropertyParams UHT_STATICS::NewProp_NotifyClass = { "NotifyClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventAddNotify_Parms, NotifyClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UAnimNotify, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams UHT_STATICS::NewProp_TimeSeconds = { "TimeSeconds", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventAddNotify_Parms, TimeSeconds), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_TrackIndex = { "TrackIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventAddNotify_Parms, TrackIndex), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventAddNotify_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NotifyClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TimeSeconds,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_TrackIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function AddNotify Property Definitions ******************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "AddNotify", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventAddNotify_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventAddNotify_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_AddNotify(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execAddNotify)
{
	P_GET_OBJECT(UAnimSequenceBase,Z_Param_Sequence);
	P_GET_OBJECT(UClass,Z_Param_NotifyClass);
	P_GET_PROPERTY(FFloatProperty,Z_Param_TimeSeconds);
	P_GET_PROPERTY(FIntProperty,Z_Param_TrackIndex);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::AddNotify(Z_Param_Sequence,Z_Param_NotifyClass,Z_Param_TimeSeconds,Z_Param_TrackIndex);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function AddNotify ********************************************

// ********** Begin Class UAnimToolset Function GetAnimInfo ****************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_GetAnimInfo_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventGetAnimInfo_Parms
	{
		UAnimSequenceBase* Sequence;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON string describing an animation sequence: length in seconds, frame rate, number of frames,\n\x09 * rate scale, loop flag, root motion flags, additive type, skeleton and number of notifies.\n\x09 * @param Sequence The animation sequence.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON string describing an animation sequence: length in seconds, frame rate, number of frames,\nrate scale, loop flag, root motion flags, additive type, skeleton and number of notifies.\n@param Sequence The animation sequence." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetAnimInfo constinit property declarations ***************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetAnimInfo constinit property declarations *****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetAnimInfo Property Definitions **************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetAnimInfo_Parms, Sequence), Z_Construct_UClass_UAnimSequenceBase, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetAnimInfo_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetAnimInfo Property Definitions ****************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "GetAnimInfo", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventGetAnimInfo_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventGetAnimInfo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_GetAnimInfo(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execGetAnimInfo)
{
	P_GET_OBJECT(UAnimSequenceBase,Z_Param_Sequence);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::GetAnimInfo(Z_Param_Sequence);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function GetAnimInfo ******************************************

// ********** Begin Class UAnimToolset Function GetBoneTrajectory **********************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_GetBoneTrajectory_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventGetBoneTrajectory_Parms
	{
		UAnimSequence* Sequence;
		FString BoneName;
		int32 StartFrame;
		int32 EndFrame;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns the position of a bone in component space (cm, relative to the mesh origin; includes root motion if the\n\x09 * root bone is animated) for a range of frames. JSON: {\"bone\",\"fps\",\"startFrame\",\"endFrame\",\"positions\":[[x,y,z],...]}.\n\x09 * Frame i is at time i / fps. Bones without an animated track use the skeleton reference pose.\n\x09 * @param Sequence The animation sequence.\n\x09 * @param BoneName Name of the bone, e.g. foot_l.\n\x09 * @param StartFrame First frame (0 based).\n\x09 * @param EndFrame Last frame inclusive; -1 means the last frame.\n\x09 */" },
#endif
		{ "CPP_Default_EndFrame", "-1" },
		{ "CPP_Default_StartFrame", "0" },
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the position of a bone in component space (cm, relative to the mesh origin; includes root motion if the\nroot bone is animated) for a range of frames. JSON: {\"bone\",\"fps\",\"startFrame\",\"endFrame\",\"positions\":[[x,y,z],...]}.\nFrame i is at time i / fps. Bones without an animated track use the skeleton reference pose.\n@param Sequence The animation sequence.\n@param BoneName Name of the bone, e.g. foot_l.\n@param StartFrame First frame (0 based).\n@param EndFrame Last frame inclusive; -1 means the last frame." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_BoneName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function GetBoneTrajectory constinit property declarations *********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FStrPropertyParams NewProp_BoneName;
	static const UECodeGen_Private::FIntPropertyParams NewProp_StartFrame;
	static const UECodeGen_Private::FIntPropertyParams NewProp_EndFrame;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetBoneTrajectory constinit property declarations ***********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetBoneTrajectory Property Definitions ********************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetBoneTrajectory_Parms, Sequence), Z_Construct_UClass_UAnimSequence, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_BoneName = { "BoneName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetBoneTrajectory_Parms, BoneName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_BoneName_MetaData), NewProp_BoneName_MetaData) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_StartFrame = { "StartFrame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetBoneTrajectory_Parms, StartFrame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_EndFrame = { "EndFrame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetBoneTrajectory_Parms, EndFrame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetBoneTrajectory_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_BoneName,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_StartFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_EndFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetBoneTrajectory Property Definitions **********************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "GetBoneTrajectory", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventGetBoneTrajectory_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventGetBoneTrajectory_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_GetBoneTrajectory(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execGetBoneTrajectory)
{
	P_GET_OBJECT(UAnimSequence,Z_Param_Sequence);
	P_GET_PROPERTY(FStrProperty,Z_Param_BoneName);
	P_GET_PROPERTY(FIntProperty,Z_Param_StartFrame);
	P_GET_PROPERTY(FIntProperty,Z_Param_EndFrame);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::GetBoneTrajectory(Z_Param_Sequence,Z_Param_BoneName,Z_Param_StartFrame,Z_Param_EndFrame);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function GetBoneTrajectory ************************************

// ********** Begin Class UAnimToolset Function GetRootMotionSummary *******************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_GetRootMotionSummary_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventGetRootMotionSummary_Parms
	{
		UAnimSequence* Sequence;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON summary of root motion measured from the first bone of the skeleton (the root): translation at the\n\x09 * start and end, total travelled distance (cm), horizontal distance, average speed in cm/s over the sequence, and\n\x09 * the average horizontal speed in cm/s. Uses the raw animation track (rate scale is NOT applied).\n\x09 * @param Sequence The animation sequence.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON summary of root motion measured from the first bone of the skeleton (the root): translation at the\nstart and end, total travelled distance (cm), horizontal distance, average speed in cm/s over the sequence, and\nthe average horizontal speed in cm/s. Uses the raw animation track (rate scale is NOT applied).\n@param Sequence The animation sequence." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetRootMotionSummary constinit property declarations ******************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetRootMotionSummary constinit property declarations ********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetRootMotionSummary Property Definitions *****************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetRootMotionSummary_Parms, Sequence), Z_Construct_UClass_UAnimSequence, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventGetRootMotionSummary_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function GetRootMotionSummary Property Definitions *******************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "GetRootMotionSummary", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventGetRootMotionSummary_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventGetRootMotionSummary_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_GetRootMotionSummary(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execGetRootMotionSummary)
{
	P_GET_OBJECT(UAnimSequence,Z_Param_Sequence);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::GetRootMotionSummary(Z_Param_Sequence);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function GetRootMotionSummary *********************************

// ********** Begin Class UAnimToolset Function ListBones ******************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_ListBones_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventListBones_Parms
	{
		UAnimSequence* Sequence;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON string listing the skeleton's bones in order: index, name, parent index, and whether the sequence\n\x09 * has an animated track for the bone.\n\x09 * @param Sequence The animation sequence.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON string listing the skeleton's bones in order: index, name, parent index, and whether the sequence\nhas an animated track for the bone.\n@param Sequence The animation sequence." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function ListBones constinit property declarations *****************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ListBones constinit property declarations *******************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ListBones Property Definitions ****************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventListBones_Parms, Sequence), Z_Construct_UClass_UAnimSequence, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventListBones_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ListBones Property Definitions ******************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "ListBones", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventListBones_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventListBones_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_ListBones(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execListBones)
{
	P_GET_OBJECT(UAnimSequence,Z_Param_Sequence);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::ListBones(Z_Param_Sequence);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function ListBones ********************************************

// ********** Begin Class UAnimToolset Function ListNotifies ***************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_ListNotifies_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventListNotifies_Parms
	{
		UAnimSequenceBase* Sequence;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns a JSON string with every notify on a sequence: name, trigger time (seconds), frame (at the sequence frame rate),\n\x09 * duration (state notifies), track index, and the notify / notify state class path.\n\x09 * @param Sequence The animation sequence.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns a JSON string with every notify on a sequence: name, trigger time (seconds), frame (at the sequence frame rate),\nduration (state notifies), track index, and the notify / notify state class path.\n@param Sequence The animation sequence." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function ListNotifies constinit property declarations **************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ListNotifies constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ListNotifies Property Definitions *************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventListNotifies_Parms, Sequence), Z_Construct_UClass_UAnimSequenceBase, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventListNotifies_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function ListNotifies Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "ListNotifies", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventListNotifies_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventListNotifies_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_ListNotifies(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execListNotifies)
{
	P_GET_OBJECT(UAnimSequenceBase,Z_Param_Sequence);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::ListNotifies(Z_Param_Sequence);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function ListNotifies *****************************************

// ********** Begin Class UAnimToolset Function RemoveNotify ***************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UFunction_UAnimToolset_RemoveNotify_Statics
struct UHT_STATICS
{
	struct AnimToolset_eventRemoveNotify_Parms
	{
		UAnimSequenceBase* Sequence;
		int32 NotifyIndex;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "AICallable", "" },
		{ "Category", "AnimToolset" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Removes the notify at the given index (see ListNotifies). Marks the asset dirty (does not save).\n\x09 * @param Sequence The animation sequence.\n\x09 * @param NotifyIndex Index in the Notifies array.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Removes the notify at the given index (see ListNotifies). Marks the asset dirty (does not save).\n@param Sequence The animation sequence.\n@param NotifyIndex Index in the Notifies array." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function RemoveNotify constinit property declarations **************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Sequence;
	static const UECodeGen_Private::FIntPropertyParams NewProp_NotifyIndex;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function RemoveNotify constinit property declarations ****************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function RemoveNotify Property Definitions *************************************
const UECodeGen_Private::FObjectPropertyParams UHT_STATICS::NewProp_Sequence = { "Sequence", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventRemoveNotify_Parms, Sequence), Z_Construct_UClass_UAnimSequenceBase, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams UHT_STATICS::NewProp_NotifyIndex = { "NotifyIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventRemoveNotify_Parms, NotifyIndex), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(AnimToolset_eventRemoveNotify_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_Sequence,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_NotifyIndex,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Function RemoveNotify Property Definitions ***************************************
const UECodeGen_Private::FFunctionParams UHT_STATICS::FuncParams = { { (FTypeConstructFunc*)Z_Construct_UClass_UAnimToolset, nullptr, "RemoveNotify", UHT_STATICS::PropPointers, UE_ARRAY_COUNT(UHT_STATICS::PropPointers), DataSizeOf<UHT_STATICS::AnimToolset_eventRemoveNotify_Parms>(), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)},  };
static_assert(sizeof(UHT_STATICS::AnimToolset_eventRemoveNotify_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAnimToolset_RemoveNotify(ETypeConstructPhase Phase)
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, UHT_STATICS::FuncParams);
	}
	return ReturnFunction;
}
#undef UHT_STATICS
DEFINE_FUNCTION(UAnimToolset::execRemoveNotify)
{
	P_GET_OBJECT(UAnimSequenceBase,Z_Param_Sequence);
	P_GET_PROPERTY(FIntProperty,Z_Param_NotifyIndex);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UAnimToolset::RemoveNotify(Z_Param_Sequence,Z_Param_NotifyIndex);
	P_NATIVE_END;
}
// ********** End Class UAnimToolset Function RemoveNotify *****************************************

// ********** Begin Class UAnimToolset *************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UAnimToolset_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/// Animation tools (read-only): sequence info, notifies, bones, bone trajectories, root motion summary.\n" },
#endif
		{ "IncludePath", "AnimToolset.h" },
		{ "ModuleRelativePath", "Public/AnimToolset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Animation tools (read-only): sequence info, notifies, bones, bone trajectories, root motion summary." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UAnimToolset constinit property declarations *****************************
// ********** End Class UAnimToolset constinit property declarations *******************************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("AddNotify"), .Pointer = &UAnimToolset::execAddNotify },
		{ .NameUTF8 = UTF8TEXT("GetAnimInfo"), .Pointer = &UAnimToolset::execGetAnimInfo },
		{ .NameUTF8 = UTF8TEXT("GetBoneTrajectory"), .Pointer = &UAnimToolset::execGetBoneTrajectory },
		{ .NameUTF8 = UTF8TEXT("GetRootMotionSummary"), .Pointer = &UAnimToolset::execGetRootMotionSummary },
		{ .NameUTF8 = UTF8TEXT("ListBones"), .Pointer = &UAnimToolset::execListBones },
		{ .NameUTF8 = UTF8TEXT("ListNotifies"), .Pointer = &UAnimToolset::execListNotifies },
		{ .NameUTF8 = UTF8TEXT("RemoveNotify"), .Pointer = &UAnimToolset::execRemoveNotify },
	};
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UAnimToolset_AddNotify, "AddNotify" }, // e2d28695adb2740ba9184d863a2aa7fdbedbd7c8
		{ &Z_Construct_UFunction_UAnimToolset_GetAnimInfo, "GetAnimInfo" }, // 1476691816332f4e86f387c0ea22c71319c39214
		{ &Z_Construct_UFunction_UAnimToolset_GetBoneTrajectory, "GetBoneTrajectory" }, // 48884371dceb592af0cc1f29479198df7c5f5f05
		{ &Z_Construct_UFunction_UAnimToolset_GetRootMotionSummary, "GetRootMotionSummary" }, // 4324f7bc38402e8251c8d1a884b0dfe2029340de
		{ &Z_Construct_UFunction_UAnimToolset_ListBones, "ListBones" }, // 82c203db2f109d0c515a749072bfcf01b15a4ac3
		{ &Z_Construct_UFunction_UAnimToolset_ListNotifies, "ListNotifies" }, // 78e905cce50e1fe27fb5c1c933d464fc07df384d
		{ &Z_Construct_UFunction_UAnimToolset_RemoveNotify, "RemoveNotify" }, // c973f3224bed395ce45d4372350701ecd2f27376
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAnimToolset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UToolsetDefinition,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_AnimToolset,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UAnimToolset,
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
static void UAnimToolset_StaticRegisterNativesUAnimToolset()
{
	UClass* Class = UAnimToolset::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, 		MakeConstArrayView(UHT_STATICS::Funcs));
}
FClassRegistrationInfo Z_Registration_Info_UClass_UAnimToolset;
UClass* Z_Construct_UClass_UAnimToolset(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UAnimToolset;
		if (!Z_Registration_Info_UClass_UAnimToolset.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("AnimToolset"),
				Z_Registration_Info_UClass_UAnimToolset.InnerSingleton,
				UAnimToolset_StaticRegisterNativesUAnimToolset,
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
		return Z_Registration_Info_UClass_UAnimToolset.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UAnimToolset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAnimToolset.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UAnimToolset.OuterSingleton;
}
#undef UHT_STATICS
UAnimToolset::UAnimToolset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UAnimToolset);
UAnimToolset::~UAnimToolset() {}
// ********** End Class UAnimToolset ***************************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h__Script_AnimToolset_Statics
struct UHT_STATICS
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UAnimToolset, TEXT("UAnimToolset"), &Z_Registration_Info_UClass_UAnimToolset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAnimToolset), 1493510392U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h__Script_AnimToolset_21be6f482429d93b765886cd460bad1c17c5f123{
	TEXT("/Script/AnimToolset"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
