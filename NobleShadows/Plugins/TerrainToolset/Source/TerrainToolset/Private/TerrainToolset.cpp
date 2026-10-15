#include "TerrainToolset.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeEditLayer.h"
#include "LandscapeLayerInfoObject.h"
#include "EngineUtils.h"
#include "WorldPartition/WorldPartition.h"
#include "LandscapeUtils.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "LandscapeProxy.h"
#include "LandscapeSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "TerrainToolset"

namespace
{
	const int32 ValidQuadsPerSection[] = { 7, 15, 31, 63, 127, 255 };
	// Matches ULandscapeEditorObject::WorldPartitionRegionSize default; larger landscapes need region volumes (not supported yet).
	const int32 MaxComponentsPerCallWorldPartition = 16;

	bool IsValidQuadsPerSection(int32 Quads)
	{
		for (int32 V : ValidQuadsPerSection)
		{
			if (V == Quads)
			{
				return true;
			}
		}
		return false;
	}

	FString ToJsonString(const TSharedRef<FJsonObject>& Object)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Object, Writer);
		return Out;
	}

	void FillSize(const TSharedRef<FJsonObject>& Obj, int32 QuadsPerSection, int32 SectionsPerComponent,
		int32 CountX, int32 CountY, double ScaleX, double ScaleY)
	{
		const int32 QuadsPerComponent = QuadsPerSection * SectionsPerComponent;
		const int64 QuadsX = (int64)QuadsPerComponent * CountX;
		const int64 QuadsY = (int64)QuadsPerComponent * CountY;
		Obj->SetNumberField(TEXT("quadsPerSection"), QuadsPerSection);
		Obj->SetNumberField(TEXT("sectionsPerComponent"), SectionsPerComponent);
		Obj->SetNumberField(TEXT("quadsPerComponent"), QuadsPerComponent);
		Obj->SetNumberField(TEXT("componentCountX"), CountX);
		Obj->SetNumberField(TEXT("componentCountY"), CountY);
		Obj->SetNumberField(TEXT("verticesX"), (double)(QuadsX + 1));
		Obj->SetNumberField(TEXT("verticesY"), (double)(QuadsY + 1));
		Obj->SetNumberField(TEXT("sizeMetersX"), QuadsX * ScaleX / 100.0);
		Obj->SetNumberField(TEXT("sizeMetersY"), QuadsY * ScaleY / 100.0);
	}
}

ALandscape* UTerrainToolset::CreateLandscape(
	const FVector& Location, const FRotator& Rotation, const FVector& Scale,
	int32 QuadsPerSection, int32 SectionsPerComponent,
	int32 ComponentCountX, int32 ComponentCountY,
	UMaterialInterface* Material, int32 WorldPartitionGridSize, const FString& HeightmapFile)
{
	if (!IsValidQuadsPerSection(QuadsPerSection))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("QuadsPerSection must be one of 7, 15, 31, 63, 127, 255."));
		return nullptr;
	}
	if (SectionsPerComponent != 1 && SectionsPerComponent != 2)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("SectionsPerComponent must be 1 or 2."));
		return nullptr;
	}
	if (ComponentCountX < 1 || ComponentCountY < 1)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("ComponentCountX and ComponentCountY must be at least 1."));
		return nullptr;
	}
	if (Scale.X <= 0.0 || Scale.Y <= 0.0 || Scale.Z == 0.0)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Scale X and Y must be positive and Z non-zero."));
		return nullptr;
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("No editor world is loaded."));
		return nullptr;
	}

	ULandscapeSubsystem* LandscapeSubsystem = World->GetSubsystem<ULandscapeSubsystem>();
	const bool bGridBased = LandscapeSubsystem && LandscapeSubsystem->IsGridBased();
	if (bGridBased && (ComponentCountX > MaxComponentsPerCallWorldPartition || ComponentCountY > MaxComponentsPerCallWorldPartition))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(
			TEXT("World Partition level: at most %d x %d components per CreateLandscape call (requested %d x %d). Larger landscapes need region volumes, which this tool does not create yet."),
			MaxComponentsPerCallWorldPartition, MaxComponentsPerCallWorldPartition, ComponentCountX, ComponentCountY));
		return nullptr;
	}

	const int32 QuadsPerComponent = SectionsPerComponent * QuadsPerSection;
	const int32 SizeX = ComponentCountX * QuadsPerComponent + 1;
	const int32 SizeY = ComponentCountY * QuadsPerComponent + 1;

	const FScopedTransaction Transaction(LOCTEXT("CreateLandscape", "Create Landscape (TerrainToolset)"));

	// Same centring offset as the Landscape tool's New Landscape panel.
	const FVector Offset = FTransform(Rotation, FVector::ZeroVector, Scale).TransformVector(
		FVector(-ComponentCountX * QuadsPerComponent / 2.0, -ComponentCountY * QuadsPerComponent / 2.0, 0.0));

	ALandscape* Landscape = World->SpawnActor<ALandscape>(Location + Offset, Rotation);
	if (!Landscape)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Failed to spawn the Landscape actor."));
		return nullptr;
	}
	Landscape->LandscapeMaterial = Material;
	Landscape->SetActorRelativeScale3D(Scale);
	Landscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((SizeX * SizeY) / (2048 * 2048) + 1), (uint32)2);

	// Empty height data = flat landscape at the default mid height.
	TArray<uint16> InitialHeights;
	if (!HeightmapFile.IsEmpty())
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *HeightmapFile) || Bytes.Num() != SizeX * SizeY * 2)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("HeightmapFile must be a raw 16-bit little-endian file of %d x %d vertices (%d bytes)."), SizeX, SizeY, SizeX * SizeY * 2));
			Landscape->Destroy();
			return nullptr;
		}
		InitialHeights.SetNumUninitialized(SizeX * SizeY);
		FMemory::Memcpy(InitialHeights.GetData(), Bytes.GetData(), Bytes.Num());
	}
	TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
	HeightDataPerLayers.Add(FGuid(), MoveTemp(InitialHeights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
	MaterialLayerDataPerLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

	Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, SectionsPerComponent, QuadsPerSection,
		HeightDataPerLayers, TEXT(""), MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive,
		TArrayView<const FLandscapeLayer>());

	ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
	if (!LandscapeInfo)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape was created but has no LandscapeInfo."));
		return Landscape;
	}
	Landscape->SetActorLabel(TEXT("Landscape"));
	LandscapeInfo->UpdateLayerInfoMap(Landscape);

	if (bGridBased && WorldPartitionGridSize > 0)
	{
		LandscapeSubsystem->ChangeGridSize(LandscapeInfo, (uint32)WorldPartitionGridSize);
	}

	return Landscape;
}

FString UTerrainToolset::GetLandscapeInfo(ALandscapeProxy* Landscape)
{
	if (!Landscape)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape is null."));
		return FString();
	}
	ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
	if (!Info)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape has no LandscapeInfo."));
		return FString();
	}

	const FVector Scale = Landscape->GetActorScale3D();
	const int32 QuadsPerComponent = Info->ComponentSizeQuads;

	// GetCompleteBounds() can be empty for a fresh landscape, so derive the footprint from the
	// component grid (keys are component base vertex coordinates) and the proxy transform.
	FBox Bounds(ForceInit);
	if (Info->XYtoComponentMap.Num() > 0 && QuadsPerComponent > 0)
	{
		FIntPoint MinKey(MAX_int32, MAX_int32), MaxKey(MIN_int32, MIN_int32);
		for (const TPair<FIntPoint, ULandscapeComponent*>& Pair : Info->XYtoComponentMap)
		{
			MinKey.X = FMath::Min(MinKey.X, Pair.Key.X);
			MinKey.Y = FMath::Min(MinKey.Y, Pair.Key.Y);
			MaxKey.X = FMath::Max(MaxKey.X, Pair.Key.X);
			MaxKey.Y = FMath::Max(MaxKey.Y, Pair.Key.Y);
		}
		// XYtoComponentMap keys are component indices (section base / component size in quads).
		const FTransform ProxyXform = Landscape->GetActorTransform();
		const double Q = (double)QuadsPerComponent;
		const FVector Lo = ProxyXform.TransformPosition(FVector(MinKey.X * Q, MinKey.Y * Q, 0.0));
		const FVector Hi = ProxyXform.TransformPosition(FVector((MaxKey.X + 1) * Q, (MaxKey.Y + 1) * Q, 0.0));
		Bounds = FBox(FVector(FMath::Min(Lo.X, Hi.X), FMath::Min(Lo.Y, Hi.Y), Lo.Z),
			FVector(FMath::Max(Lo.X, Hi.X), FMath::Max(Lo.Y, Hi.Y), Lo.Z));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("name"), Landscape->GetActorNameOrLabel());
	Obj->SetNumberField(TEXT("quadsPerSection"), Info->SubsectionSizeQuads);
	Obj->SetNumberField(TEXT("sectionsPerComponent"), Info->ComponentNumSubsections);
	Obj->SetNumberField(TEXT("quadsPerComponent"), QuadsPerComponent);
	Obj->SetNumberField(TEXT("componentCount"), Info->XYtoComponentMap.Num());
	if (QuadsPerComponent > 0 && Scale.X > 0.0 && Scale.Y > 0.0)
	{
		const double CompsX = (Bounds.Max.X - Bounds.Min.X) / (QuadsPerComponent * Scale.X);
		const double CompsY = (Bounds.Max.Y - Bounds.Min.Y) / (QuadsPerComponent * Scale.Y);
		Obj->SetNumberField(TEXT("componentCountX"), FMath::RoundToDouble(CompsX));
		Obj->SetNumberField(TEXT("componentCountY"), FMath::RoundToDouble(CompsY));
		Obj->SetNumberField(TEXT("verticesX"), FMath::RoundToDouble(CompsX) * QuadsPerComponent + 1);
		Obj->SetNumberField(TEXT("verticesY"), FMath::RoundToDouble(CompsY) * QuadsPerComponent + 1);
	}
	Obj->SetNumberField(TEXT("scaleX"), Scale.X);
	Obj->SetNumberField(TEXT("scaleY"), Scale.Y);
	Obj->SetNumberField(TEXT("scaleZ"), Scale.Z);
	Obj->SetNumberField(TEXT("sizeMetersX"), (Bounds.Max.X - Bounds.Min.X) / 100.0);
	Obj->SetNumberField(TEXT("sizeMetersY"), (Bounds.Max.Y - Bounds.Min.Y) / 100.0);
	Obj->SetNumberField(TEXT("minX"), Bounds.Min.X);
	Obj->SetNumberField(TEXT("minY"), Bounds.Min.Y);
	Obj->SetNumberField(TEXT("minZ"), Bounds.Min.Z);
	Obj->SetNumberField(TEXT("maxX"), Bounds.Max.X);
	Obj->SetNumberField(TEXT("maxY"), Bounds.Max.Y);
	Obj->SetNumberField(TEXT("maxZ"), Bounds.Max.Z);
	return ToJsonString(Obj);
}

FString UTerrainToolset::PlanLandscape(int32 QuadsPerSection, int32 SectionsPerComponent,
	int32 ComponentCountX, int32 ComponentCountY, float ScaleXY)
{
	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> ValidQuads;
	for (int32 V : ValidQuadsPerSection)
	{
		ValidQuads.Add(MakeShared<FJsonValueNumber>(V));
	}
	Obj->SetArrayField(TEXT("validQuadsPerSection"), ValidQuads);
	Obj->SetStringField(TEXT("validSectionsPerComponent"), TEXT("1 or 2"));
	Obj->SetNumberField(TEXT("maxComponentsPerCallWorldPartition"), MaxComponentsPerCallWorldPartition);

	TArray<FString> Problems;
	if (!IsValidQuadsPerSection(QuadsPerSection)) { Problems.Add(TEXT("invalid QuadsPerSection")); }
	if (SectionsPerComponent != 1 && SectionsPerComponent != 2) { Problems.Add(TEXT("invalid SectionsPerComponent")); }
	if (ComponentCountX < 1 || ComponentCountY < 1) { Problems.Add(TEXT("component counts must be >= 1")); }
	if (ScaleXY <= 0.f) { Problems.Add(TEXT("ScaleXY must be > 0")); }

	if (Problems.Num() == 0)
	{
		FillSize(Obj, QuadsPerSection, SectionsPerComponent, ComponentCountX, ComponentCountY, ScaleXY, ScaleXY);
	}
	Obj->SetStringField(TEXT("problems"), Problems.Num() ? FString::Join(Problems, TEXT("; ")) : TEXT("none"));
	return ToJsonString(Obj);
}

// ---------------------------------------------------------------------------------------------------------------------
// Heights
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
	constexpr int32 MaxVertsPerCall = 65536;
	constexpr double HeightScale = 1.0 / 128.0; // LANDSCAPE_ZSCALE; stored uint16 mid value is 32768.

	struct FHeightTarget
	{
		ULandscapeInfo* Info = nullptr;
		ALandscape* Landscape = nullptr;
		FIntRect Extent;      // in vertices; Max is inclusive
		double ZScale = 100.0;
		double ZOrigin = 0.0;
		FTransform Xform;

		int32 VertsX() const { return Extent.Max.X - Extent.Min.X + 1; }
		int32 VertsY() const { return Extent.Max.Y - Extent.Min.Y + 1; }
		float ToWorld(uint16 V) const { return (float)(ZOrigin + ((double)V - 32768.0) * HeightScale * ZScale); }
		uint16 FromWorld(double Cm) const
		{
			const double Raw = (Cm - ZOrigin) / (HeightScale * ZScale) + 32768.0;
			return (uint16)FMath::Clamp(FMath::RoundToInt(Raw), 0, 65535);
		}
		FVector2D VertToWorldXY(int32 X, int32 Y) const
		{
			const FVector P = Xform.TransformPosition(FVector(X, Y, 0.0));
			return FVector2D(P.X, P.Y);
		}
	};

	bool ResolveTarget(ALandscapeProxy* Proxy, FHeightTarget& Out)
	{
		if (!Proxy)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape is null."));
			return false;
		}
		Out.Info = Proxy->GetLandscapeInfo();
		Out.Landscape = Out.Info ? Out.Info->LandscapeActor.Get() : nullptr;
		if (!Out.Info || !Out.Landscape)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape has no loaded main Landscape actor."));
			return false;
		}
		if (!Out.Info->GetLandscapeExtent(Out.Extent))
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape has no components."));
			return false;
		}
		Out.Xform = Out.Landscape->GetActorTransform();
		Out.ZScale = Out.Xform.GetScale3D().Z;
		Out.ZOrigin = Out.Xform.GetLocation().Z;
		if (FMath::IsNearlyZero(Out.ZScale))
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Landscape Z scale is zero."));
			return false;
		}
		return true;
	}

	void ReadRect(const FHeightTarget& T, int32 X1, int32 Y1, int32 X2, int32 Y2, TArray<uint16>& Data)
	{
		Data.SetNumZeroed((X2 - X1 + 1) * (Y2 - Y1 + 1));
		FHeightmapAccessor<false> Accessor(T.Info);
		int32 A = X1, B = Y1, C = X2, D = Y2;
		Accessor.GetData(A, B, C, D, Data.GetData());
	}

	void WriteRect(const FHeightTarget& T, int32 X1, int32 Y1, int32 X2, int32 Y2, const TArray<uint16>& Data, const FText& Label)
	{
		ALandscape* Landscape = T.Landscape;
		const FScopedTransaction Transaction(Label);
		// A landscape with edit layers recomposites its final heightmap from the layers, so edits must go into a real layer.
		FGuid LayerGuid;
		if (Landscape->HasLayersContent())
		{
			const TArray<const ULandscapeEditLayerBase*> EditLayers = Landscape->GetEditLayersConst();
			if (EditLayers.Num() > 0 && EditLayers[0])
			{
				LayerGuid = EditLayers[0]->GetGuid();
			}
		}
		FScopedSetLandscapeEditingLayer Scope(Landscape, LayerGuid, [Landscape]
		{
			Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All);
		});
		FHeightmapAccessor<false> Accessor(T.Info);
		Accessor.SetData(X1, Y1, X2, Y2, Data.GetData());
		Accessor.Flush();

		// Make sure the edited components and their proxy actors are saved with the level.
		for (const TPair<FIntPoint, ULandscapeComponent*>& Pair : T.Info->XYtoComponentMap)
		{
			if (Pair.Value)
			{
				Pair.Value->MarkPackageDirty();
				if (ALandscapeProxy* Proxy = Pair.Value->GetLandscapeProxy())
				{
					Proxy->MarkPackageDirty();
				}
			}
		}
	}

	bool CheckRect(const FHeightTarget& T, int32 MinX, int32 MinY, int32 SizeX, int32 SizeY)
	{
		if (SizeX < 1 || SizeY < 1 || (int64)SizeX * SizeY > MaxVertsPerCall)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Rectangle must be at least 1x1 and at most %d vertices (got %d x %d)."), MaxVertsPerCall, SizeX, SizeY));
			return false;
		}
		if (MinX < T.Extent.Min.X || MinY < T.Extent.Min.Y || MinX + SizeX - 1 > T.Extent.Max.X || MinY + SizeY - 1 > T.Extent.Max.Y)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Rectangle is outside the landscape vertex range X %d..%d, Y %d..%d."),
				T.Extent.Min.X, T.Extent.Max.X, T.Extent.Min.Y, T.Extent.Max.Y));
			return false;
		}
		return true;
	}

	// Applies Fn(WorldX, WorldY, CurrentWorldZ) -> NewWorldZ over the whole landscape, in row bands.
	template<typename FnType>
	void ModifyAll(const FHeightTarget& T, const FText& Label, FnType Fn)
	{
		const int32 W = T.VertsX();
		const int32 H = T.VertsY();
		TArray<uint16> Data;
		ReadRect(T, T.Extent.Min.X, T.Extent.Min.Y, T.Extent.Max.X, T.Extent.Max.Y, Data);
		for (int32 Y = 0; Y < H; ++Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				const FVector2D P = T.VertToWorldXY(T.Extent.Min.X + X, T.Extent.Min.Y + Y);
				uint16& V = Data[Y * W + X];
				V = T.FromWorld(Fn(P.X, P.Y, T.ToWorld(V)));
			}
		}
		WriteRect(T, T.Extent.Min.X, T.Extent.Min.Y, T.Extent.Max.X, T.Extent.Max.Y, Data, Label);
	}
}

void UTerrainToolset::SetFlat(ALandscapeProxy* Landscape, float HeightCm)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return; }
	ModifyAll(T, LOCTEXT("SetFlat", "Terrain: Set Flat"), [HeightCm](double, double, double) { return (double)HeightCm; });
}

TArray<float> UTerrainToolset::GetHeights(ALandscapeProxy* Landscape, int32 MinVertX, int32 MinVertY, int32 SizeX, int32 SizeY)
{
	TArray<float> Out;
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T) || !CheckRect(T, MinVertX, MinVertY, SizeX, SizeY)) { return Out; }
	TArray<uint16> Data;
	ReadRect(T, MinVertX, MinVertY, MinVertX + SizeX - 1, MinVertY + SizeY - 1, Data);
	Out.Reserve(Data.Num());
	for (uint16 V : Data) { Out.Add(T.ToWorld(V)); }
	return Out;
}

void UTerrainToolset::SetHeights(ALandscapeProxy* Landscape, int32 MinVertX, int32 MinVertY, int32 SizeX, int32 SizeY, const TArray<float>& HeightsCm)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T) || !CheckRect(T, MinVertX, MinVertY, SizeX, SizeY)) { return; }
	if (HeightsCm.Num() != SizeX * SizeY)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("HeightsCm has %d values but SizeX*SizeY = %d."), HeightsCm.Num(), SizeX * SizeY));
		return;
	}
	TArray<uint16> Data;
	Data.Reserve(HeightsCm.Num());
	for (float H : HeightsCm) { Data.Add(T.FromWorld(H)); }
	WriteRect(T, MinVertX, MinVertY, MinVertX + SizeX - 1, MinVertY + SizeY - 1, Data, LOCTEXT("SetHeights", "Terrain: Set Heights"));
}

void UTerrainToolset::AddHill(ALandscapeProxy* Landscape, float CenterX, float CenterY, float RadiusCm, float HeightCm, float FalloffPower)
{
	if (RadiusCm <= 0.f)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("RadiusCm must be > 0."));
		return;
	}
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return; }
	const double Power = FMath::Max(0.1, (double)FalloffPower);
	ModifyAll(T, LOCTEXT("AddHill", "Terrain: Add Hill"), [=](double X, double Y, double Z)
	{
		const double D = FMath::Sqrt(FMath::Square(X - CenterX) + FMath::Square(Y - CenterY)) / RadiusCm;
		if (D >= 1.0) { return Z; }
		// Smooth bump: (1 - d^2)^Power style falloff.
		const double Fall = FMath::Pow(1.0 - FMath::Square(D), Power);
		return Z + HeightCm * Fall;
	});
}

void UTerrainToolset::AddNoise(ALandscapeProxy* Landscape, float AmplitudeCm, float WavelengthCm, int32 Octaves, int32 Seed)
{
	if (WavelengthCm <= 0.f)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("WavelengthCm must be > 0."));
		return;
	}
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return; }
	const int32 Oct = FMath::Clamp(Octaves, 1, 8);
	const FVector2D SeedOffset(Seed * 137.31, Seed * 71.17);
	ModifyAll(T, LOCTEXT("AddNoise", "Terrain: Add Noise"), [=](double X, double Y, double Z)
	{
		double Sum = 0.0, Amp = AmplitudeCm, Freq = 1.0 / WavelengthCm;
		for (int32 O = 0; O < Oct; ++O)
		{
			Sum += Amp * FMath::PerlinNoise2D(FVector2D(X * Freq + SeedOffset.X + O * 19.7, Y * Freq + SeedOffset.Y + O * 7.3));
			Amp *= 0.5;
			Freq *= 2.0;
		}
		return Z + Sum;
	});
}

void UTerrainToolset::ImportHeightmapFile(ALandscapeProxy* Landscape, const FString& FilePath, bool bFlipY)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return; }
	const int32 W = T.VertsX();
	const int32 H = T.VertsY();

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Could not read file: %s"), *FilePath));
		return;
	}

	TArray<uint16> Data;
	const FString Ext = FPaths::GetExtension(FilePath).ToLower();
	if (Ext == TEXT("png"))
	{
		IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
		if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()))
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Could not decode the PNG."));
			return;
		}
		if (Wrapper->GetBitDepth() != 16 || Wrapper->GetWidth() != W || Wrapper->GetHeight() != H)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("PNG must be 16-bit grayscale %d x %d (got %d-bit %d x %d)."),
				W, H, Wrapper->GetBitDepth(), Wrapper->GetWidth(), Wrapper->GetHeight()));
			return;
		}
		TArray64<uint8> Raw;
		if (!Wrapper->GetRaw(ERGBFormat::Gray, 16, Raw) || Raw.Num() != (int64)W * H * 2)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("PNG is not 16-bit grayscale."));
			return;
		}
		Data.SetNumUninitialized(W * H);
		FMemory::Memcpy(Data.GetData(), Raw.GetData(), (SIZE_T)W * H * 2);
	}
	else
	{
		if (Bytes.Num() != W * H * 2)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Raw heightmap must be %d x %d x 2 = %d bytes (got %d)."), W, H, W * H * 2, Bytes.Num()));
			return;
		}
		Data.SetNumUninitialized(W * H);
		FMemory::Memcpy(Data.GetData(), Bytes.GetData(), (SIZE_T)W * H * 2);
	}

	if (bFlipY)
	{
		for (int32 Y = 0; Y < H / 2; ++Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				Swap(Data[Y * W + X], Data[(H - 1 - Y) * W + X]);
			}
		}
	}
	WriteRect(T, T.Extent.Min.X, T.Extent.Min.Y, T.Extent.Max.X, T.Extent.Max.Y, Data, LOCTEXT("ImportHeightmap", "Terrain: Import Heightmap"));
}

int32 UTerrainToolset::LoadWorldRegion(float MinX, float MinY, float MaxX, float MaxY)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	UWorldPartition* WorldPartition = World ? World->GetWorldPartition() : nullptr;
	if (!WorldPartition)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("The editor world is not a World Partition world."));
		return 0;
	}
	WorldPartition->LoadLastLoadedRegions(TArray<FBox>{ FBox(FVector(MinX, MinY, -HALF_WORLD_MAX), FVector(MaxX, MaxY, HALF_WORLD_MAX)) });
	int32 Count = 0;
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It) { ++Count; }
	return Count;
}

FString UTerrainToolset::CreateLayerInfos(ALandscapeProxy* Landscape, const FString& PackagePath, const TArray<FString>& LayerNames)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return FString(); }
	if (!PackagePath.StartsWith(TEXT("/Game/")))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("PackagePath must start with /Game/."));
		return FString();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateLayerInfos", "Terrain: Create Layer Infos"));
	TArray<TSharedPtr<FJsonValue>> Paths;
	for (const FString& Name : LayerNames)
	{
		const FString AssetName = Name + TEXT("_LayerInfo");
		const FString ObjectPath = PackagePath / AssetName + TEXT(".") + AssetName;
		ULandscapeLayerInfoObject* LayerInfo = LoadObject<ULandscapeLayerInfoObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!LayerInfo)
		{
			LayerInfo = UE::Landscape::CreateTargetLayerInfo(FName(*Name), PackagePath, AssetName);
		}
		if (!LayerInfo) { continue; }
		T.Info->CreateTargetLayerSettingsFor(LayerInfo);
		T.Landscape->UpdateTargetLayer(FName(*Name), FLandscapeTargetLayerSettings(LayerInfo));
		Paths.Add(MakeShared<FJsonValueString>(LayerInfo->GetPathName()));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetArrayField(TEXT("layerInfos"), Paths);
	return ToJsonString(Obj);
}

FString UTerrainToolset::PaintZoneMap(ALandscapeProxy* Landscape, const FString& FilePath, const TArray<FString>& LayerNames)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return FString(); }
	const int32 W = T.VertsX();
	const int32 H = T.VertsY();

	TArray<uint8> Index;
	if (!FFileHelper::LoadFileToArray(Index, *FilePath) || Index.Num() != W * H)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Zone map must be a raw 8-bit file of %d x %d = %d bytes."), W, H, W * H));
		return FString();
	}

	const FScopedTransaction Transaction(LOCTEXT("PaintZoneMap", "Terrain: Paint Zone Map"));
	ALandscape* LandscapeActor = T.Landscape;
	FGuid EditLayerGuid;
	int32 NumEditLayers = 0;
	const bool bHasLayers = LandscapeActor->HasLayersContent();
	if (bHasLayers)
	{
		const TArray<const ULandscapeEditLayerBase*> EditLayers = LandscapeActor->GetEditLayersConst();
		NumEditLayers = EditLayers.Num();
		if (NumEditLayers > 0 && EditLayers[0]) { EditLayerGuid = EditLayers[0]->GetGuid(); }
	}
	FScopedSetLandscapeEditingLayer LayerScope(LandscapeActor, EditLayerGuid, [LandscapeActor]
	{
		LandscapeActor->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Weightmap_All);
	});
	for (int32 LayerIdx = 0; LayerIdx < LayerNames.Num(); ++LayerIdx)
	{
		ULandscapeLayerInfoObject* LayerInfo = T.Info->GetLayerInfoByName(FName(*LayerNames[LayerIdx]));
		if (!LayerInfo)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("No layer info registered for '%s'."), *LayerNames[LayerIdx]));
			return FString();
		}
		TArray<uint8> Weights;
		Weights.SetNumUninitialized(W * H);
		for (int32 I = 0; I < W * H; ++I)
		{
			Weights[I] = (Index[I] == LayerIdx) ? 255 : 0;
		}
		TAlphamapAccessor<false> Accessor(T.Info, LayerInfo);
		Accessor.SetData(T.Extent.Min.X, T.Extent.Min.Y, T.Extent.Max.X, T.Extent.Max.Y, Weights.GetData(), ELandscapeLayerPaintingRestriction::None);
		Accessor.Flush();
	}

	for (const TPair<FIntPoint, ULandscapeComponent*>& Pair : T.Info->XYtoComponentMap)
	{
		if (Pair.Value)
		{
			Pair.Value->MarkPackageDirty();
			if (ALandscapeProxy* Proxy = Pair.Value->GetLandscapeProxy())
			{
				Proxy->MarkPackageDirty();
			}
		}
	}

	TSharedRef<FJsonObject> Diag = MakeShared<FJsonObject>();
	Diag->SetBoolField(TEXT("hasLayersContent"), bHasLayers);
	Diag->SetNumberField(TEXT("editLayers"), NumEditLayers);
	Diag->SetNumberField(TEXT("components"), T.Info->XYtoComponentMap.Num());
	if (ULandscapeComponent* C0 = T.Info->XYtoComponentMap.FindRef(FIntPoint(8, 8)))
	{
		Diag->SetNumberField(TEXT("centerComponentAllocations"), C0->GetWeightmapLayerAllocations(EditLayerGuid).Num());
		Diag->SetNumberField(TEXT("centerComponentAllocationsFinal"), C0->GetWeightmapLayerAllocations(FGuid()).Num());
	}
	return ToJsonString(Diag);
}

FString UTerrainToolset::ReadLayerBlobs(ALandscapeProxy* Landscape, int32 StepVerts, int32 Threshold, int32 MaxBlobsPerLayer)
{
	FHeightTarget T;
	if (!ResolveTarget(Landscape, T)) { return FString(); }
	StepVerts = FMath::Max(1, StepVerts);
	Threshold = FMath::Clamp(Threshold, 1, 255);
	MaxBlobsPerLayer = FMath::Max(1, MaxBlobsPerLayer);

	const int32 W = T.VertsX();
	const int32 H = T.VertsY();
	const int32 GW = (W + StepVerts - 1) / StepVerts;
	const int32 GH = (H + StepVerts - 1) / StepVerts;
	const FVector Scale = T.Xform.GetScale3D();
	const double CellAreaM2 = (StepVerts * FMath::Abs(Scale.X) / 100.0) * (StepVerts * FMath::Abs(Scale.Y) / 100.0);

	TArray<TSharedPtr<FJsonValue>> LayerArray;
	for (const FLandscapeInfoLayerSettings& Settings : T.Info->Layers)
	{
		ULandscapeLayerInfoObject* LayerInfo = Settings.LayerInfoObj;
		if (!LayerInfo) { continue; }

		TArray<uint8> Weights;
		Weights.SetNumZeroed(W * H);
		{
			TAlphamapAccessor<false> Accessor(T.Info, LayerInfo);
			int32 A = T.Extent.Min.X, B = T.Extent.Min.Y, C = T.Extent.Max.X, D = T.Extent.Max.Y;
			Accessor.GetData(A, B, C, D, Weights.GetData());
		}

		TBitArray<> Painted(false, GW * GH);
		int64 PaintedCount = 0;
		for (int32 GY = 0; GY < GH; ++GY)
		{
			for (int32 GX = 0; GX < GW; ++GX)
			{
				const int32 VX = FMath::Min(GX * StepVerts, W - 1);
				const int32 VY = FMath::Min(GY * StepVerts, H - 1);
				if (Weights[VY * W + VX] >= Threshold)
				{
					Painted[GY * GW + GX] = true;
					++PaintedCount;
				}
			}
		}

		struct FBlob { int64 Count = 0; int32 MinX = MAX_int32, MinY = MAX_int32, MaxX = MIN_int32, MaxY = MIN_int32; double SumX = 0, SumY = 0; };
		TArray<FBlob> Blobs;
		TBitArray<> Visited(false, GW * GH);
		TArray<FIntPoint> Stack;
		for (int32 Start = 0; Start < GW * GH; ++Start)
		{
			if (!Painted[Start] || Visited[Start]) { continue; }
			FBlob Blob;
			Stack.Reset();
			Stack.Add(FIntPoint(Start % GW, Start / GW));
			Visited[Start] = true;
			while (Stack.Num())
			{
				const FIntPoint P = Stack.Pop(EAllowShrinking::No);
				++Blob.Count;
				Blob.MinX = FMath::Min(Blob.MinX, P.X); Blob.MaxX = FMath::Max(Blob.MaxX, P.X);
				Blob.MinY = FMath::Min(Blob.MinY, P.Y); Blob.MaxY = FMath::Max(Blob.MaxY, P.Y);
				Blob.SumX += P.X; Blob.SumY += P.Y;
				static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
				for (const FIntPoint& Dir : Dirs)
				{
					const int32 NX = P.X + Dir.X, NY = P.Y + Dir.Y;
					if (NX < 0 || NY < 0 || NX >= GW || NY >= GH) { continue; }
					const int32 Idx = NY * GW + NX;
					if (Painted[Idx] && !Visited[Idx])
					{
						Visited[Idx] = true;
						Stack.Add(FIntPoint(NX, NY));
					}
				}
			}
			Blobs.Add(Blob);
		}
		Blobs.Sort([](const FBlob& L, const FBlob& R) { return L.Count > R.Count; });

		auto CellToWorld = [&](double GX, double GY)
		{
			return T.VertToWorldXY(T.Extent.Min.X + FMath::RoundToInt(GX * StepVerts), T.Extent.Min.Y + FMath::RoundToInt(GY * StepVerts));
		};

		TArray<TSharedPtr<FJsonValue>> BlobArray;
		for (int32 I = 0; I < Blobs.Num() && I < MaxBlobsPerLayer; ++I)
		{
			const FBlob& B = Blobs[I];
			const FVector2D Lo = CellToWorld(B.MinX, B.MinY);
			const FVector2D Hi = CellToWorld(B.MaxX, B.MaxY);
			const FVector2D Ce = CellToWorld(B.SumX / B.Count, B.SumY / B.Count);
			TSharedRef<FJsonObject> BO = MakeShared<FJsonObject>();
			BO->SetNumberField(TEXT("areaM2"), B.Count * CellAreaM2);
			BO->SetNumberField(TEXT("minX_m"), FMath::Min(Lo.X, Hi.X) / 100.0);
			BO->SetNumberField(TEXT("minY_m"), FMath::Min(Lo.Y, Hi.Y) / 100.0);
			BO->SetNumberField(TEXT("maxX_m"), FMath::Max(Lo.X, Hi.X) / 100.0);
			BO->SetNumberField(TEXT("maxY_m"), FMath::Max(Lo.Y, Hi.Y) / 100.0);
			BO->SetNumberField(TEXT("centerX_m"), Ce.X / 100.0);
			BO->SetNumberField(TEXT("centerY_m"), Ce.Y / 100.0);
			BlobArray.Add(MakeShared<FJsonValueObject>(BO));
		}

		TSharedRef<FJsonObject> LO = MakeShared<FJsonObject>();
		LO->SetStringField(TEXT("layer"), LayerInfo->GetLayerName().ToString());
		LO->SetNumberField(TEXT("totalAreaM2"), PaintedCount * CellAreaM2);
		LO->SetNumberField(TEXT("blobCount"), Blobs.Num());
		LO->SetArrayField(TEXT("blobs"), BlobArray);
		LayerArray.Add(MakeShared<FJsonValueObject>(LO));
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("stepVerts"), StepVerts);
	Root->SetNumberField(TEXT("threshold"), Threshold);
	Root->SetNumberField(TEXT("cellAreaM2"), CellAreaM2);
	Root->SetArrayField(TEXT("layers"), LayerArray);
	return ToJsonString(Root);
}

#undef LOCTEXT_NAMESPACE



// rebuild stamp 2026-10-10T23:04:57.9315257-07:00
