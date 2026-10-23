#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"

/**
 * Epic Control Rig style control shapes: closed line loops, never solid cubes or spheres. Shapes are defined in unit space
 * (-1..1) in the local YZ plane, so the ring "faces" local X (the bone axis once the shape is aligned to the bone).
 */
namespace AnimStitchControlShapes
{
	/** The loops of a shape. Auto / legacy Box / legacy Sphere fall back to Circle; resolve them with Resolve first. */
	const TArray<TArray<FVector>>& GetLoops(EAnimStitchControlShape Shape);

	/** The curve that suits a control's role: circle for FK joints, square for the pelvis and IK targets, hexagon for hands and feet, diamond for pole vectors, cross for the root offset. */
	EAnimStitchControlShape DefaultFor(FName Bone, EAnimStitchTrackKind Kind);

	/** The shape actually drawn: the style's own choice, or the role default when it is Auto (or one of the pre-2.0 Box / Sphere values). */
	EAnimStitchControlShape Resolve(const FAnimStitchControlStyle& Style, FName Bone, EAnimStitchTrackKind Kind);

	/** Display name of a user-pickable shape. */
	FText GetDisplayName(EAnimStitchControlShape Shape);
}
