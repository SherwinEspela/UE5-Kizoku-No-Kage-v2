// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "WidgetToolset.h"

#ifdef WIDGETTOOLSET_WidgetToolset_generated_h
#error "WidgetToolset.generated.h already included, missing '#pragma once' in WidgetToolset.h"
#endif
#define WIDGETTOOLSET_WidgetToolset_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UClass;
class UNiagaraParameterCollection;
class UUserWidget;
class UWidget;
class UWidgetBlueprint;

// ********** Begin Class UWidgetToolset ***********************************************************
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execSetParameterCollectionDefault); \
	DECLARE_FUNCTION(execCreateWidgetBlueprint); \
	DECLARE_FUNCTION(execGetParameterCollectionInfo); \
	DECLARE_FUNCTION(execCreateParameterCollection); \
	DECLARE_FUNCTION(execCompileWidgetBlueprint); \
	DECLARE_FUNCTION(execBindWidgetEvent); \
	DECLARE_FUNCTION(execSetSlotProperty); \
	DECLARE_FUNCTION(execSetWidgetProperty); \
	DECLARE_FUNCTION(execSetWidgetIsVariable); \
	DECLARE_FUNCTION(execRenameWidget); \
	DECLARE_FUNCTION(execMoveWidget); \
	DECLARE_FUNCTION(execRemoveWidget); \
	DECLARE_FUNCTION(execAddWidget); \
	DECLARE_FUNCTION(execFindWidgetClasses); \
	DECLARE_FUNCTION(execListWidgetAnimations); \
	DECLARE_FUNCTION(execGetWidgetProperties); \
	DECLARE_FUNCTION(execGetWidgetTree);


struct Z_Construct_UClass_UWidgetToolset_Statics;
WIDGETTOOLSET_API UClass* Z_Construct_UClass_UWidgetToolset(ETypeConstructPhase);

#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UWidgetToolset_Statics; \
	friend WIDGETTOOLSET_API UClass* ::Z_Construct_UClass_UWidgetToolset(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UWidgetToolset, UToolsetDefinition, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/WidgetToolset"), Z_Construct_UClass_UWidgetToolset) \
	DECLARE_SERIALIZER(UWidgetToolset)


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UWidgetToolset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UWidgetToolset(UWidgetToolset&&) = delete; \
	UWidgetToolset(const UWidgetToolset&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UWidgetToolset); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UWidgetToolset); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UWidgetToolset) \
	NO_API virtual ~UWidgetToolset();


#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_18_PROLOG
#define FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_INCLASS_NO_PURE_DECLS \
	FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h_21_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UWidgetToolset;

// ********** End Class UWidgetToolset *************************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_PROJECTS_Unreal_KIZOKU_NO_KAGE_V2_REPO_NobleShadows_Plugins_WidgetToolset_Source_WidgetToolset_Public_WidgetToolset_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
