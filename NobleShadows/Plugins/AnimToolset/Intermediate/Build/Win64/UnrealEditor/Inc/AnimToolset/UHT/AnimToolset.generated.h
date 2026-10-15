// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AnimToolset.h"

#ifdef ANIMTOOLSET_AnimToolset_generated_h
#error "AnimToolset.generated.h already included, missing '#pragma once' in AnimToolset.h"
#endif
#define ANIMTOOLSET_AnimToolset_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UAnimNotify;
class UAnimSequence;
class UAnimSequenceBase;
class UClass;

// ********** Begin Class UAnimToolset *************************************************************
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execRemoveNotify); \
	DECLARE_FUNCTION(execAddNotify); \
	DECLARE_FUNCTION(execGetRootMotionSummary); \
	DECLARE_FUNCTION(execGetBoneTrajectory); \
	DECLARE_FUNCTION(execListBones); \
	DECLARE_FUNCTION(execListNotifies); \
	DECLARE_FUNCTION(execGetAnimInfo);


struct Z_Construct_UClass_UAnimToolset_Statics;
ANIMTOOLSET_API UClass* Z_Construct_UClass_UAnimToolset(ETypeConstructPhase);

#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UAnimToolset_Statics; \
	friend ANIMTOOLSET_API UClass* ::Z_Construct_UClass_UAnimToolset(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UAnimToolset, UToolsetDefinition, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/AnimToolset"), Z_Construct_UClass_UAnimToolset) \
	DECLARE_SERIALIZER(UAnimToolset)


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAnimToolset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UAnimToolset(UAnimToolset&&) = delete; \
	UAnimToolset(const UAnimToolset&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAnimToolset); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAnimToolset); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAnimToolset) \
	NO_API virtual ~UAnimToolset();


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_13_PROLOG
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_INCLASS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UAnimToolset;

// ********** End Class UAnimToolset ***************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_AnimToolset_Source_AnimToolset_Public_AnimToolset_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
