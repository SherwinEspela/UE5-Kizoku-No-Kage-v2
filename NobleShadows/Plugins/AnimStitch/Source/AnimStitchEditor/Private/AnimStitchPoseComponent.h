#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "AnimStitchPoseComponent.generated.h"

/** Poseable mesh that can be driven directly by a skeleton-space local pose from FAnimStitchEvaluator. */
UCLASS()
class UAnimStitchPoseComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	/** Applies a local pose indexed by USkeleton reference-skeleton bone (mesh bones are matched by name). */
	void ApplySkeletonPose(const USkeleton* Skeleton, const TArray<FTransform>& SkeletonLocalPose);

private:
	TWeakObjectPtr<const USkeleton> CachedSkeleton;
	TWeakObjectPtr<USkinnedAsset> CachedAsset;
	/** Mesh bone index -> skeleton bone index. */
	TArray<int32> MeshToSkeleton;
};
