#include "WidgetToolset.h"

#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraTypes.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NamedSlotInterface.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Dom/JsonObject.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"
#include "MovieSceneTrack.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UObjectIterator.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintOperationUtils.h"

namespace
{
	FString ToJsonString(const TSharedRef<FJsonObject>& Obj)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj, Writer);
		return Out;
	}

	FString ErrorJson(const FString& Message)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("ok"), false);
		Obj->SetStringField(TEXT("error"), Message);
		return ToJsonString(Obj);
	}

	FString OkJson(TSharedRef<FJsonObject> Obj)
	{
		Obj->SetBoolField(TEXT("ok"), true);
		return ToJsonString(Obj);
	}

	bool CheckBlueprint(UWidgetBlueprint* BP, FString& OutError)
	{
		if (!BP)
		{
			OutError = TEXT("WidgetBlueprint is null");
			return false;
		}
		if (!BP->WidgetTree)
		{
			OutError = TEXT("Widget blueprint has no widget tree");
			return false;
		}
		return true;
	}

	UWidget* FindWidgetByName(UWidgetBlueprint* BP, const FString& Name, FString& OutError)
	{
		UWidget* Widget = BP->WidgetTree->FindWidget(FName(*Name));
		if (!Widget)
		{
			OutError = FString::Printf(TEXT("No widget named '%s' in %s"), *Name, *BP->GetName());
		}
		return Widget;
	}

	TSharedRef<FJsonObject> WidgetToJson(const UWidget* Widget)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Widget->GetName());
		Obj->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
		Obj->SetBoolField(TEXT("isVariable"), Widget->bIsVariable);
		Obj->SetStringField(TEXT("visibility"), StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Widget->GetVisibility())));
		if (Widget->Slot)
		{
			Obj->SetStringField(TEXT("slotClass"), Widget->Slot->GetClass()->GetName());
		}

		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			TArray<TSharedPtr<FJsonValue>> Children;
			for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
			{
				if (const UWidget* Child = Panel->GetChildAt(i))
				{
					Children.Add(MakeShared<FJsonValueObject>(WidgetToJson(Child)));
				}
			}
			if (Children.Num() > 0)
			{
				Obj->SetArrayField(TEXT("children"), Children);
			}
		}

		if (const INamedSlotInterface* Host = Cast<const INamedSlotInterface>(Widget))
		{
			TArray<FName> SlotNames;
			Host->GetSlotNames(SlotNames);
			TArray<TSharedPtr<FJsonValue>> Named;
			for (const FName SlotName : SlotNames)
			{
				if (const UWidget* Content = Host->GetContentForSlot(SlotName))
				{
					TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
					Entry->SetStringField(TEXT("slot"), SlotName.ToString());
					Entry->SetObjectField(TEXT("content"), WidgetToJson(Content));
					Named.Add(MakeShared<FJsonValueObject>(Entry));
				}
			}
			if (Named.Num() > 0)
			{
				Obj->SetArrayField(TEXT("namedSlots"), Named);
			}
		}
		return Obj;
	}

	/** Exports editable properties of Obj as name -> text; only non-default ones unless bAll. */
	TSharedRef<FJsonObject> DumpProperties(UObject* Obj, bool bAll)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		UObject* Defaults = Obj->GetClass()->GetDefaultObject();
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* Prop = *It;
			if (!Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_Deprecated))
			{
				continue;
			}
			if (CastField<FMulticastDelegateProperty>(Prop) || CastField<FDelegateProperty>(Prop))
			{
				continue;
			}
			if (!bAll && Prop->Identical_InContainer(Obj, Defaults))
			{
				continue;
			}
			FString Value;
			Prop->ExportText_InContainer(0, Value, Obj, bAll ? nullptr : Defaults, Obj, PPF_None);
			Out->SetStringField(Prop->GetName(), Value);
		}
		return Out;
	}

	FString SetPropertyOnObject(UWidgetBlueprint* BP, UObject* Target, const FString& PropertyName, const FString& ValueText)
	{
		FProperty* Prop = Target->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (!Prop)
		{
			return ErrorJson(FString::Printf(TEXT("Class %s has no property '%s'"), *Target->GetClass()->GetName(), *PropertyName));
		}
		if (!Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasAnyPropertyFlags(CPF_EditConst))
		{
			return ErrorJson(FString::Printf(TEXT("Property '%s' is not editable"), *PropertyName));
		}

		Target->Modify();
		BP->Modify();
		const TCHAR* Result = Prop->ImportText_InContainer(*ValueText, Target, Target, PPF_None);
		if (!Result)
		{
			return ErrorJson(FString::Printf(TEXT("Could not parse '%s' as a value for %s (%s)"), *ValueText, *PropertyName, *Prop->GetCPPType()));
		}

		FPropertyChangedEvent ChangedEvent(Prop);
		Target->PostEditChangeProperty(ChangedEvent);
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

		FString NewValue;
		Prop->ExportText_InContainer(0, NewValue, Target, nullptr, Target, PPF_None);
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("property"), PropertyName);
		Obj->SetStringField(TEXT("value"), NewValue);
		return OkJson(Obj);
	}
}

// ------------------------------------------------------------------- read

FString UWidgetToolset::GetWidgetTree(UWidgetBlueprint* WidgetBlueprint)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("blueprint"), WidgetBlueprint->GetPathName());
	Obj->SetStringField(TEXT("parentClass"), WidgetBlueprint->ParentClass ? WidgetBlueprint->ParentClass->GetPathName() : FString());

	if (const UWidget* Root = WidgetBlueprint->WidgetTree->RootWidget)
	{
		Obj->SetObjectField(TEXT("root"), WidgetToJson(Root));
	}

	TArray<UWidget*> All;
	WidgetBlueprint->WidgetTree->GetAllWidgets(All);
	Obj->SetNumberField(TEXT("widgetCount"), All.Num());

	TArray<TSharedPtr<FJsonValue>> Anims;
	for (const UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (Anim)
		{
			Anims.Add(MakeShared<FJsonValueString>(Anim->GetName()));
		}
	}
	Obj->SetArrayField(TEXT("animations"), Anims);
	return OkJson(Obj);
}

FString UWidgetToolset::GetWidgetProperties(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, bool bAllProperties)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Widget->GetName());
	Obj->SetStringField(TEXT("class"), Widget->GetClass()->GetPathName());
	Obj->SetObjectField(TEXT("widget"), DumpProperties(Widget, bAllProperties));
	if (Widget->Slot)
	{
		Obj->SetStringField(TEXT("slotClass"), Widget->Slot->GetClass()->GetName());
		Obj->SetObjectField(TEXT("slot"), DumpProperties(Widget->Slot, bAllProperties));
	}
	return OkJson(Obj);
}

FString UWidgetToolset::ListWidgetAnimations(UWidgetBlueprint* WidgetBlueprint)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}

	TArray<TSharedPtr<FJsonValue>> Anims;
	for (const UWidgetAnimation* Anim : WidgetBlueprint->Animations)
	{
		if (!Anim)
		{
			continue;
		}
		TSharedRef<FJsonObject> A = MakeShared<FJsonObject>();
		A->SetStringField(TEXT("name"), Anim->GetName());
		A->SetNumberField(TEXT("startSeconds"), Anim->GetStartTime());
		A->SetNumberField(TEXT("endSeconds"), Anim->GetEndTime());

		const UMovieScene* Scene = Anim->GetMovieScene();
		TArray<TSharedPtr<FJsonValue>> Bindings;
		for (const FWidgetAnimationBinding& Binding : Anim->GetBindings())
		{
			TSharedRef<FJsonObject> B = MakeShared<FJsonObject>();
			B->SetStringField(TEXT("widget"), Binding.WidgetName.ToString());
			if (!Binding.SlotWidgetName.IsNone())
			{
				B->SetStringField(TEXT("slotOf"), Binding.SlotWidgetName.ToString());
			}
			B->SetBoolField(TEXT("isRoot"), Binding.bIsRootWidget);

			TArray<TSharedPtr<FJsonValue>> Tracks;
			if (Scene)
			{
				for (const FMovieSceneBinding& SceneBinding : Scene->GetBindings())
				{
					if (SceneBinding.GetObjectGuid() != Binding.AnimationGuid)
					{
						continue;
					}
					for (const UMovieSceneTrack* Track : SceneBinding.GetTracks())
					{
						if (!Track)
						{
							continue;
						}
						TSharedRef<FJsonObject> T = MakeShared<FJsonObject>();
						T->SetStringField(TEXT("class"), Track->GetClass()->GetName());
#if WITH_EDITORONLY_DATA
						T->SetStringField(TEXT("display"), Track->GetDisplayName().ToString());
#endif
						Tracks.Add(MakeShared<FJsonValueObject>(T));
					}
				}
			}
			B->SetArrayField(TEXT("tracks"), Tracks);
			Bindings.Add(MakeShared<FJsonValueObject>(B));
		}
		A->SetArrayField(TEXT("bindings"), Bindings);
		Anims.Add(MakeShared<FJsonValueObject>(A));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetArrayField(TEXT("animations"), Anims);
	return OkJson(Obj);
}

FString UWidgetToolset::FindWidgetClasses(const FString& NameFilter)
{
	TArray<TSharedPtr<FJsonValue>> Classes;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UWidget::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}
		if (!NameFilter.IsEmpty() && !Class->GetName().Contains(NameFilter))
		{
			continue;
		}
		Classes.Add(MakeShared<FJsonValueString>(Class->GetPathName()));
	}
	Classes.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B) { return A->AsString() < B->AsString(); });

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("count"), Classes.Num());
	Obj->SetArrayField(TEXT("classes"), Classes);
	return OkJson(Obj);
}

// ------------------------------------------------------------------ write

FString UWidgetToolset::AddWidget(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass, const FString& WidgetName, const FString& ParentName, int32 ChildIndex)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	if (!WidgetClass || WidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return ErrorJson(TEXT("WidgetClass is null or abstract"));
	}
	if (WidgetClass->ClassGeneratedBy == WidgetBlueprint)
	{
		return ErrorJson(TEXT("A widget blueprint cannot contain itself"));
	}

	FName NewName = NAME_None;
	if (!WidgetName.IsEmpty())
	{
		NewName = FName(*WidgetName);
		if (WidgetBlueprint->WidgetTree->FindWidget(NewName) || FBlueprintEditorUtils::FindNewVariableIndex(WidgetBlueprint, NewName) != INDEX_NONE)
		{
			return ErrorJson(FString::Printf(TEXT("The name '%s' is already used in this blueprint"), *WidgetName));
		}
	}

	UWidget* Parent = nullptr;
	if (!ParentName.IsEmpty())
	{
		Parent = FindWidgetByName(WidgetBlueprint, ParentName, Error);
		if (!Parent)
		{
			return ErrorJson(Error);
		}
	}

	FText ErrorText;
	if (!FWidgetBlueprintOperationUtils::CanAddToParent(WidgetBlueprint, Parent, ErrorText))
	{
		return ErrorJson(ErrorText.ToString());
	}

	WidgetBlueprint->Modify();
	UWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewName);
	if (!NewWidget)
	{
		return ErrorJson(TEXT("Could not construct the widget"));
	}
	if (!FWidgetBlueprintOperationUtils::AddWidget(WidgetBlueprint, NewWidget, Parent, ChildIndex, ErrorText))
	{
		FWidgetBlueprintOperationUtils::RemoveTransientWidgetFromTree(WidgetBlueprint, NewWidget);
		return ErrorJson(ErrorText.ToString());
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), NewWidget->GetName());
	Obj->SetStringField(TEXT("class"), NewWidget->GetClass()->GetPathName());
	Obj->SetStringField(TEXT("parent"), Parent ? Parent->GetName() : FString());
	return OkJson(Obj);
}

FString UWidgetToolset::RemoveWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}

	FText ErrorText;
	WidgetBlueprint->Modify();
	if (!FWidgetBlueprintOperationUtils::RemoveWidget(WidgetBlueprint, Widget, ErrorText))
	{
		return ErrorJson(ErrorText.IsEmpty() ? TEXT("Remove failed") : ErrorText.ToString());
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("removed"), WidgetName);
	return OkJson(Obj);
}

FString UWidgetToolset::MoveWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& NewParentName, int32 ChildIndex)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}
	UWidget* ParentWidget = FindWidgetByName(WidgetBlueprint, NewParentName, Error);
	if (!ParentWidget)
	{
		return ErrorJson(Error);
	}
	UPanelWidget* NewParent = Cast<UPanelWidget>(ParentWidget);
	if (!NewParent)
	{
		return ErrorJson(FString::Printf(TEXT("'%s' is not a panel widget"), *NewParentName));
	}

	FText ErrorText;
	WidgetBlueprint->Modify();
	if (!FWidgetBlueprintOperationUtils::MoveWidget(WidgetBlueprint, Widget, NewParent, ChildIndex, ErrorText))
	{
		return ErrorJson(ErrorText.IsEmpty() ? TEXT("Move failed") : ErrorText.ToString());
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("moved"), WidgetName);
	Obj->SetStringField(TEXT("newParent"), NewParentName);
	return OkJson(Obj);
}

FString UWidgetToolset::RenameWidget(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& NewName)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}

	FText ErrorText;
	if (!FWidgetBlueprintOperationUtils::VerifyWidgetRename(WidgetBlueprint, Widget, FText::FromString(NewName), ErrorText))
	{
		return ErrorJson(ErrorText.ToString());
	}
	WidgetBlueprint->Modify();
	if (!FWidgetBlueprintOperationUtils::RenameWidget(WidgetBlueprint, Widget, NewName))
	{
		return ErrorJson(TEXT("Rename failed"));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("oldName"), WidgetName);
	Obj->SetStringField(TEXT("newName"), Widget->GetName());
	return OkJson(Obj);
}

FString UWidgetToolset::SetWidgetIsVariable(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, bool bIsVariable)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}

	WidgetBlueprint->Modify();
	FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(WidgetBlueprint, Widget, bIsVariable);

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("widget"), WidgetName);
	Obj->SetBoolField(TEXT("isVariable"), Widget->bIsVariable);
	return OkJson(Obj);
}

FString UWidgetToolset::SetWidgetProperty(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& PropertyName, const FString& ValueText)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}
	return SetPropertyOnObject(WidgetBlueprint, Widget, PropertyName, ValueText);
}

FString UWidgetToolset::SetSlotProperty(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& PropertyName, const FString& ValueText)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}
	if (!Widget->Slot)
	{
		return ErrorJson(FString::Printf(TEXT("Widget '%s' has no slot (it is the root or in a named slot)"), *WidgetName));
	}
	return SetPropertyOnObject(WidgetBlueprint, Widget->Slot, PropertyName, ValueText);
}

FString UWidgetToolset::BindWidgetEvent(UWidgetBlueprint* WidgetBlueprint, const FString& WidgetName, const FString& EventName)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}
	UWidget* Widget = FindWidgetByName(WidgetBlueprint, WidgetName, Error);
	if (!Widget)
	{
		return ErrorJson(Error);
	}
	if (!Widget->bIsVariable)
	{
		return ErrorJson(FString::Printf(TEXT("Widget '%s' is not a variable; call SetWidgetIsVariable first"), *WidgetName));
	}

	FText ErrorText;
	if (!FWidgetBlueprintOperationUtils::BindToEventProperty(WidgetBlueprint, FName(*EventName), Widget->GetFName(), Widget->GetClass(), false, ErrorText))
	{
		return ErrorJson(ErrorText.IsEmpty() ? TEXT("Bind failed") : ErrorText.ToString());
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("widget"), WidgetName);
	Obj->SetStringField(TEXT("event"), EventName);
	return OkJson(Obj);
}

FString UWidgetToolset::CompileWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
{
	FString Error;
	if (!CheckBlueprint(WidgetBlueprint, Error))
	{
		return ErrorJson(Error);
	}

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint, EBlueprintCompileOptions::None, &Results);

	TArray<TSharedPtr<FJsonValue>> Messages;
	for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
	{
		Messages.Add(MakeShared<FJsonValueString>(Message->ToText().ToString()));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("errors"), Results.NumErrors);
	Obj->SetNumberField(TEXT("warnings"), Results.NumWarnings);
	Obj->SetBoolField(TEXT("upToDate"), WidgetBlueprint->Status == BS_UpToDate || WidgetBlueprint->Status == BS_UpToDateWithWarnings);
	Obj->SetArrayField(TEXT("messages"), Messages);
	return OkJson(Obj);
}

// ------------------------------------------------- niagara parameter collections

namespace
{
	/** Last dotted segment of a parameter name: "NPC.NPC_Foo.Bar" -> "Bar". */
	FString ShortParameterName(const FNiagaraVariable& Parameter)
	{
		const FString Full = Parameter.GetName().ToString();
		int32 DotIndex = INDEX_NONE;
		return Full.FindLastChar(TEXT('.'), DotIndex) ? Full.Mid(DotIndex + 1) : Full;
	}

	bool FindCollectionParameter(UNiagaraParameterCollection* Collection, const FString& Name, FNiagaraVariable& OutParameter)
	{
		for (const FNiagaraVariable& Parameter : Collection->GetParameters())
		{
			if (ShortParameterName(Parameter) == Name || Parameter.GetName().ToString() == Name)
			{
				OutParameter = Parameter;
				return true;
			}
		}
		return false;
	}
}

FString UWidgetToolset::CreateParameterCollection(const FString& FolderPath, const FString& AssetName, const TArray<FString>& FloatParameters, const TArray<FString>& VectorParameters)
{
	if (FolderPath.IsEmpty() || AssetName.IsEmpty())
	{
		return ErrorJson(TEXT("FolderPath and AssetName are required"));
	}

	FString Folder = FolderPath;
	Folder.RemoveFromEnd(TEXT("/"));
	const FString PackageName = Folder + TEXT("/") + AssetName;
	if (FindPackage(nullptr, *PackageName) || FPackageName::DoesPackageExist(PackageName))
	{
		return ErrorJson(FString::Printf(TEXT("An asset already exists at %s"), *PackageName));
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return ErrorJson(TEXT("Could not create the package"));
	}
	UNiagaraParameterCollection* Collection = NewObject<UNiagaraParameterCollection>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
	if (!Collection)
	{
		return ErrorJson(TEXT("Could not create the collection"));
	}
	Collection->RefreshCompileId();

	for (const FString& Name : FloatParameters)
	{
		Collection->AddParameter(Collection->CollectionParameterFromFriendlyParameter(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(*Name))));
	}
	for (const FString& Name : VectorParameters)
	{
		Collection->AddParameter(Collection->CollectionParameterFromFriendlyParameter(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), FName(*Name))));
	}

	FAssetRegistryModule::AssetCreated(Collection);
	Package->MarkPackageDirty();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("path"), Collection->GetPathName());
	Obj->SetStringField(TEXT("namespace"), Collection->GetFullNamespaceName().ToString());
	Obj->SetNumberField(TEXT("parameterCount"), Collection->GetParameters().Num());
	return OkJson(Obj);
}

FString UWidgetToolset::GetParameterCollectionInfo(UNiagaraParameterCollection* Collection)
{
	if (!Collection)
	{
		return ErrorJson(TEXT("Collection is null"));
	}
	UNiagaraParameterCollectionInstance* Instance = Collection->GetDefaultInstance();

	TArray<TSharedPtr<FJsonValue>> Params;
	for (const FNiagaraVariable& Parameter : Collection->GetParameters())
	{
		const FString Friendly = Parameter.GetName().ToString();
		TSharedRef<FJsonObject> P = MakeShared<FJsonObject>();
		P->SetStringField(TEXT("name"), ShortParameterName(Parameter));
		P->SetStringField(TEXT("fullName"), Friendly);
		if (Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
		{
			P->SetStringField(TEXT("type"), TEXT("float"));
			if (Instance)
			{
				P->SetNumberField(TEXT("default"), Instance->GetFloatParameter(Friendly));
			}
		}
		else if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def())
		{
			P->SetStringField(TEXT("type"), TEXT("vector3"));
			if (Instance)
			{
				const FVector V = Instance->GetVectorParameter(Friendly);
				TArray<TSharedPtr<FJsonValue>> Components;
				Components.Add(MakeShared<FJsonValueNumber>(V.X));
				Components.Add(MakeShared<FJsonValueNumber>(V.Y));
				Components.Add(MakeShared<FJsonValueNumber>(V.Z));
				P->SetArrayField(TEXT("default"), Components);
			}
		}
		else
		{
			P->SetStringField(TEXT("type"), Parameter.GetType().GetName());
		}
		Params.Add(MakeShared<FJsonValueObject>(P));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("path"), Collection->GetPathName());
	Obj->SetStringField(TEXT("namespace"), Collection->GetFullNamespaceName().ToString());
	Obj->SetArrayField(TEXT("parameters"), Params);
	return OkJson(Obj);
}

FString UWidgetToolset::SetParameterCollectionDefault(UNiagaraParameterCollection* Collection, const FString& ParameterName, float X, float Y, float Z)
{
	if (!Collection)
	{
		return ErrorJson(TEXT("Collection is null"));
	}
	FNiagaraVariable Parameter;
	if (!FindCollectionParameter(Collection, ParameterName, Parameter))
	{
		return ErrorJson(FString::Printf(TEXT("No parameter named '%s'"), *ParameterName));
	}
	UNiagaraParameterCollectionInstance* Instance = Collection->GetDefaultInstance();
	if (!Instance)
	{
		return ErrorJson(TEXT("Collection has no default instance"));
	}

	Collection->Modify();
	Instance->Modify();
	const FString FullName = Parameter.GetName().ToString();
	if (Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
	{
		Instance->SetFloatParameter(FullName, X);
	}
	else if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def())
	{
		Instance->SetVectorParameter(FullName, FVector(X, Y, Z));
	}
	else
	{
		return ErrorJson(TEXT("Only float and vector3 parameters are supported"));
	}
	Collection->MarkPackageDirty();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("parameter"), ParameterName);
	return OkJson(Obj);
}

FString UWidgetToolset::CreateWidgetBlueprint(const FString& FolderPath, const FString& AssetName, TSubclassOf<UUserWidget> ParentClass, TSubclassOf<UWidget> RootWidgetClass)
{
	if (FolderPath.IsEmpty() || AssetName.IsEmpty())
	{
		return ErrorJson(TEXT("FolderPath and AssetName are required"));
	}
	FString Folder = FolderPath;
	Folder.RemoveFromEnd(TEXT("/"));
	const FString PackageName = Folder + TEXT("/") + AssetName;
	if (FindPackage(nullptr, *PackageName) || FPackageName::DoesPackageExist(PackageName))
	{
		return ErrorJson(FString::Printf(TEXT("An asset already exists at %s"), *PackageName));
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return ErrorJson(TEXT("Could not create the package"));
	}
	TSubclassOf<UUserWidget> Parent = ParentClass ? ParentClass : TSubclassOf<UUserWidget>(UUserWidget::StaticClass());
	UWidgetBlueprint* Blueprint = FWidgetBlueprintOperationUtils::CreateWidgetBlueprint(Package, FName(*AssetName), BPTYPE_Normal, Parent, RootWidgetClass, TEXT("WidgetToolset"), true);
	if (!Blueprint)
	{
		return ErrorJson(TEXT("Widget blueprint creation failed"));
	}
	Blueprint->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("path"), Blueprint->GetPathName());
	Obj->SetStringField(TEXT("parentClass"), Parent->GetPathName());
	return OkJson(Obj);
}
