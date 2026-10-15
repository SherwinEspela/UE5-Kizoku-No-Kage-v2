#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "TerrainToolset.generated.h"

class ALandscape;
class ALandscapeProxy;
class UMaterialInterface;

/// Terrain tools: create landscapes from explicit parameters and read back their configuration.
UCLASS(BlueprintType)
class UTerrainToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new flat landscape in the current level with explicit size parameters.
	 * Total quads per side = ComponentCount * SectionsPerComponent * QuadsPerSection. Vertices per side = quads + 1.
	 * The landscape is centred on Location (same behaviour as the Landscape tool's New Landscape panel).
	 * In a World Partition level the landscape is limited to 16x16 components per call.
	 *
	 * @param Location World location of the landscape centre.
	 * @param Rotation World rotation.
	 * @param Scale Landscape actor scale (X/Y = cm per quad, Z = height scale; 100,100,100 is the engine default).
	 * @param QuadsPerSection Quads per section edge. Must be one of 7, 15, 31, 63, 127, 255.
	 * @param SectionsPerComponent Sections per component edge: 1 (1x1) or 2 (2x2).
	 * @param ComponentCountX Number of components along X.
	 * @param ComponentCountY Number of components along Y.
	 * @param Material Optional landscape material.
	 * @param WorldPartitionGridSize Streaming grid size in components (only used in World Partition levels; engine default 2).
	 * @param HeightmapFile Optional absolute path to a raw 16-bit little-endian heightmap (.r16) with exactly one value per vertex; used as the initial heights (32768 = actor Z).
	 * @return The created landscape actor.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static ALandscape* CreateLandscape(
		const FVector& Location, const FRotator& Rotation, const FVector& Scale,
		int32 QuadsPerSection = 63, int32 SectionsPerComponent = 1,
		int32 ComponentCountX = 8, int32 ComponentCountY = 8,
		UMaterialInterface* Material = nullptr, int32 WorldPartitionGridSize = 2, const FString& HeightmapFile = TEXT(""));

	/**
	 * Returns a JSON string describing a landscape: quads per section, sections per component,
	 * quads per component, component counts, total quads/vertices, scale, bounds in cm and metres.
	 * @param Landscape The landscape (or proxy) to inspect.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static FString GetLandscapeInfo(ALandscapeProxy* Landscape);

	/**
	 * Returns a JSON string with the valid parameter values and the size in metres/vertices that a
	 * given parameter combination would produce, for planning before calling CreateLandscape.
	 * @param QuadsPerSection Quads per section edge to evaluate.
	 * @param SectionsPerComponent Sections per component edge to evaluate.
	 * @param ComponentCountX Components along X to evaluate.
	 * @param ComponentCountY Components along Y to evaluate.
	 * @param ScaleXY cm per quad to evaluate.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static FString PlanLandscape(int32 QuadsPerSection = 63, int32 SectionsPerComponent = 1,
		int32 ComponentCountX = 8, int32 ComponentCountY = 8, float ScaleXY = 100.f);

	// ---- Heights. All heights are WORLD Z in cm. Vertex coordinates are landscape vertex indices (0..vertices-1).
	// Heights are stored as uint16 (1/128 units * Z scale), so the usable range is about +-256 * ScaleZ/100 metres around the landscape actor's Z.

	/**
	 * Sets the whole landscape to one constant world height.
	 * @param Landscape The landscape.
	 * @param HeightCm World Z in cm.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static void SetFlat(ALandscapeProxy* Landscape, float HeightCm);

	/**
	 * Returns heights (world Z cm) for a rectangle of vertices, row-major (X varies fastest).
	 * Limited to 65536 vertices per call.
	 * @param Landscape The landscape.
	 * @param MinVertX First vertex X.
	 * @param MinVertY First vertex Y.
	 * @param SizeX Number of vertices along X.
	 * @param SizeY Number of vertices along Y.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static TArray<float> GetHeights(ALandscapeProxy* Landscape, int32 MinVertX, int32 MinVertY, int32 SizeX, int32 SizeY);

	/**
	 * Writes heights (world Z cm) into a rectangle of vertices, row-major (X varies fastest).
	 * Limited to 65536 vertices per call. Values outside the representable range are clamped.
	 * @param Landscape The landscape.
	 * @param MinVertX First vertex X.
	 * @param MinVertY First vertex Y.
	 * @param SizeX Number of vertices along X.
	 * @param SizeY Number of vertices along Y.
	 * @param HeightsCm SizeX*SizeY world heights in cm.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static void SetHeights(ALandscapeProxy* Landscape, int32 MinVertX, int32 MinVertY, int32 SizeX, int32 SizeY, const TArray<float>& HeightsCm);

	/**
	 * Adds (or subtracts, if negative) a smooth round hill.
	 * @param Landscape The landscape.
	 * @param CenterX World X of the hill centre (cm).
	 * @param CenterY World Y of the hill centre (cm).
	 * @param RadiusCm Hill radius in cm.
	 * @param HeightCm Height added at the centre (cm); negative digs.
	 * @param FalloffPower 1 = linear cone, 2 = smooth (default), higher = steeper sides, flatter top.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static void AddHill(ALandscapeProxy* Landscape, float CenterX, float CenterY, float RadiusCm, float HeightCm, float FalloffPower = 2.f);

	/**
	 * Adds fractal Perlin noise over the whole landscape.
	 * @param Landscape The landscape.
	 * @param AmplitudeCm Peak amplitude of the first octave (cm); noise is centred on zero.
	 * @param WavelengthCm Wavelength of the first octave (cm).
	 * @param Octaves Number of octaves (1-8); each halves wavelength and amplitude.
	 * @param Seed Offsets the noise field.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static void AddNoise(ALandscapeProxy* Landscape, float AmplitudeCm, float WavelengthCm, int32 Octaves = 4, int32 Seed = 0);

	/**
	 * Imports a heightmap file over the whole landscape. Supports .r16/.raw (16-bit little-endian, exact vertex count)
	 * and 16-bit grayscale .png. The file values map straight to the landscape's stored heights (the same as the
	 * Landscape tool's Import), so 32768 = landscape actor Z.
	 * @param Landscape The landscape.
	 * @param FilePath Absolute path to the file.
	 * @param bFlipY Flip the image vertically on import.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static void ImportHeightmapFile(ALandscapeProxy* Landscape, const FString& FilePath, bool bFlipY = false);

	/**
	 * Loads a world-space box of the World Partition editor world so its actors (e.g. landscape proxies) are loaded.
	 * Returns the number of landscape proxies loaded afterwards.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static int32 LoadWorldRegion(float MinX, float MinY, float MaxX, float MaxY);

	// ---- Paint layers.

	/**
	 * Creates weight-blended Landscape Layer Info assets (one per name) in a content folder and registers them as
	 * target layers on the landscape so they can be painted. Existing layer infos with the same asset name are reused.
	 * The new assets are left dirty; save them afterwards. Returns a JSON string listing the asset paths.
	 * @param Landscape The landscape.
	 * @param PackagePath Content folder, e.g. /Game/AI_Experiments/Shared/ZoneCoding.
	 * @param LayerNames Layer names exactly as used by the landscape material (e.g. Zone_Path).
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static FString CreateLayerInfos(ALandscapeProxy* Landscape, const FString& PackagePath, const TArray<FString>& LayerNames);

	/**
	 * Paints paint layers from a zone index map: a raw 8-bit file with one byte per landscape vertex (row-major, X fastest),
	 * where each byte is an index into LayerNames. Every listed layer gets weight 255 where its index matches and 0 elsewhere,
	 * so each vertex ends up fully on one layer. Layers not in LayerNames are untouched. Layer infos must already exist.
	 * @param Landscape The landscape.
	 * @param FilePath Absolute path to the .r8 index map (exactly one byte per vertex).
	 * @param LayerNames Layer names, indexed by the byte values in the file.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static FString PaintZoneMap(ALandscapeProxy* Landscape, const FString& FilePath, const TArray<FString>& LayerNames);

	/**
	 * Reads the painted weightmaps and returns a JSON string: for every paint layer, its covered area and its
	 * connected regions (blobs) as world-space bounds in metres, centroid and area, largest first.
	 * Sampling is every StepVerts vertices (4 = one sample per 4 m at scale 100).
	 * @param Landscape The landscape.
	 * @param StepVerts Sample spacing in vertices (>=1).
	 * @param Threshold Weight (0-255) at or above which a sample counts as painted.
	 * @param MaxBlobsPerLayer Maximum regions reported per layer.
	 */
	UFUNCTION(meta = (AICallable), Category = "TerrainToolset")
	static FString ReadLayerBlobs(ALandscapeProxy* Landscape, int32 StepVerts = 4, int32 Threshold = 128, int32 MaxBlobsPerLayer = 40);
};

