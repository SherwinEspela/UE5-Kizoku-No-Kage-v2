#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "WidgetToolset.generated.h"

class UNiagaraParameterCollection;
class UUserWidget;
class UWidget;
class UWidgetBlueprint;

/**
 * UMG / CommonUI widget blueprint tools. Read tools first, then write tools. Write tools mark the blueprint
 * modified but never save; call CompileWidgetBlueprint and then save with the asset tools.
 * All results are JSON strings: {"ok":true,...} or {"ok":false,"error":"..."}.
 */
UCLASS(BlueprintType)
class UWidgetToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------------- read

	/**
	 * Returns the widget tree of a widget blueprint as nested JSON: per widget its name, class path, bIsVariable,
	 * visibility, slot class, panel children ("children") and named-slot content ("namedSlots"). Also lists the
	 * parent class and the animation names. Does not descend into nested user widgets.
	 * @param WidgetBlueprint The widget blueprint.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString GetWidgetTree(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Returns the editable properties of one widget and of its panel slot as exported text. By default only
	 * properties that differ from the class default are returned.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the widget in the tree.
	 * @param bAllProperties True to return every editable property, not only non-default ones.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString GetWidgetProperties(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, bool bAllProperties = false);

	/**
	 * Lists the widget animations of a blueprint: name, start/end time in seconds, the widgets they are bound to
	 * and the track class / display name for every binding. Read-only.
	 * @param WidgetBlueprint The widget blueprint.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString ListWidgetAnimations(UWidgetBlueprint* WidgetBlueprint);

	/**
	 * Lists loaded, non-abstract UWidget subclasses (native and loaded blueprint classes) whose name contains the
	 * filter, with the class path to pass to AddWidget. Widget blueprints that are not loaded are not listed; for
	 * those pass the generated class path (e.g. /Game/.../WBP_Foo.WBP_Foo_C) directly to AddWidget.
	 * @param NameFilter Case-insensitive substring of the class name; empty returns all.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString FindWidgetClasses(const FString& NameFilter);

	// --------------------------------------------------------------- write

	/**
	 * Adds a new widget to the tree. ParentName empty makes it the root if the tree is empty. Marks the blueprint
	 * modified (does not save or compile).
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetClass Class of the widget, e.g. /Script/UMG.Overlay or a user widget generated class.
	 * @param WidgetName Desired unique name; empty lets the engine pick one.
	 * @param ParentName Name of the parent panel widget; empty for root.
	 * @param ChildIndex Index in the parent; -1 appends.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString AddWidget(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass, const FString& WidgetName, const FString& ParentName, int32 ChildIndex = -1);

	/**
	 * Removes a widget and all of its children from the tree (cleans references). Marks the blueprint modified.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the widget to remove.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString RemoveWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName);

	/**
	 * Moves a widget to a new parent panel at an index. Marks the blueprint modified.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Widget to move.
	 * @param NewParentName Name of the new parent panel widget.
	 * @param ChildIndex Index in the new parent; -1 appends.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString MoveWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& NewParentName, int32 ChildIndex = -1);

	/**
	 * Renames a widget (also renames its variable and updates references). Marks the blueprint modified.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Current name.
	 * @param NewName New name; must be unique and a valid variable name.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString RenameWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& NewName);

	/**
	 * Sets or clears the "Is Variable" flag of a widget so it can be referenced in graphs. Marks the blueprint modified.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the widget.
	 * @param bIsVariable The new value.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString SetWidgetIsVariable(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, bool bIsVariable);

	/**
	 * Sets one editable property on a widget from exported text (same format GetWidgetProperties returns), e.g.
	 * Visibility = Collapsed, or a struct as "(R=1,G=0,B=0,A=1)". Returns the property value after the change.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the widget.
	 * @param PropertyName Property name, e.g. Visibility, Text, RenderOpacity.
	 * @param ValueText Value in Unreal text format.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString SetWidgetProperty(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& PropertyName, const FString& ValueText);

	/**
	 * Sets one editable property on the widget's panel slot (padding, anchors, alignment, size...) from exported text.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the child widget whose slot to edit.
	 * @param PropertyName Slot property, e.g. Padding, HorizontalAlignment, LayoutData.
	 * @param ValueText Value in Unreal text format.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString SetSlotProperty(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& PropertyName, const FString& ValueText);

	/**
	 * Adds an event node bound to a widget's delegate (like clicking "+" next to OnClicked in the Details panel).
	 * The widget must be marked as a variable. Then fill in the graph with the Blueprint tools.
	 * @param WidgetBlueprint The widget blueprint.
	 * @param WidgetName Name of the widget (must be a variable).
	 * @param EventName Delegate property name, e.g. OnClicked, OnHovered.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString BindWidgetEvent(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& EventName);

	/**
	 * Compiles the widget blueprint and returns error / warning counts and messages. Does not save.
	 * @param WidgetBlueprint The widget blueprint.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString CompileWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);

	// ---------------------------------------------------- niagara parameter collections

	/**
	 * Creates a new Niagara Parameter Collection asset with float and vector3 parameters. Blueprint can write these
	 * every tick (Get Niagara Parameter Collection, then Set Float / Vector Parameter) and Niagara systems read
	 * them as NPC_<AssetName>.<Param>. The asset is created in memory (marked dirty); save it with the asset tools.
	 * @param FolderPath Content folder, e.g. /Game/NobleShadows/UI.
	 * @param AssetName Asset name, e.g. NPC_EnemyIndicator. The collection namespace is derived from it.
	 * @param FloatParameters Names of float parameters to add (friendly names, e.g. Proximity01).
	 * @param VectorParameters Names of vector3 parameters to add (friendly names, e.g. LeanDirection).
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString CreateParameterCollection(const FString& FolderPath, const FString& AssetName, const TArray<FString>& FloatParameters, const TArray<FString>& VectorParameters);

	/**
	 * Lists a parameter collection's namespace and parameters (friendly name, type, current default value).
	 * @param Collection The Niagara Parameter Collection asset.
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString GetParameterCollectionInfo(UNiagaraParameterCollection* Collection);

	/**
	 * Sets the default value of a float or vector3 parameter on a collection (the value Niagara uses until Blueprint
	 * overrides it). For a vector pass X, Y, Z; for a float only X is used.
	 * @param Collection The Niagara Parameter Collection asset.
	 * @param ParameterName Friendly parameter name.
	 * @param X Float value, or vector X.
	 * @param Y Vector Y (ignored for floats).
	 * @param Z Vector Z (ignored for floats).
	 */
	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString CreateWidgetBlueprint(const FString& FolderPath, const FString& AssetName, TSubclassOf<UUserWidget> ParentClass, TSubclassOf<UWidget> RootWidgetClass);

	UFUNCTION(meta = (AICallable), Category = "WidgetToolset")
	static FString SetParameterCollectionDefault(UNiagaraParameterCollection* Collection, const FString& ParameterName, float X, float Y = 0.0f, float Z = 0.0f);
};
