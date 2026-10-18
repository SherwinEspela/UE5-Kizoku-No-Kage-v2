#include "AnimStitchPoseComponent.h"
#include "Animation/Skeleton.h"
#include "ReferenceSkeleton.h"

void UAnimStitchPoseComponent::ApplySkeletonPose(const USkeleton* Skeleton, const TArray<FTransform>& SkeletonLocalPose)
{
	USkinnedAsset* Asset = GetSkinnedAsset();
	if (!Skeleton || !Asset)
	{
		return;
	}
	const FReferenceSkeleton& MeshRef = Asset->GetRefSkeleton();
	const int32 NumMeshBones = MeshRef.GetNum();

	if (CachedSkeleton.Get() != Skeleton || CachedAsset.Get() != Asset || MeshToSkeleton.Num() != NumMeshBones)
	{
		CachedSkeleton = Skeleton;
		CachedAsset = Asset;
		MeshToSkeleton.SetNum(NumMeshBones);
		for (int32 i = 0; i < NumMeshBones; ++i)
		{
			MeshToSkeleton[i] = Skeleton->GetReferenceSkeleton().FindBoneIndex(MeshRef.GetBoneName(i));
		}
	}

	if (BoneSpaceTransforms.Num() != NumMeshBones)
	{
		BoneSpaceTransforms = MeshRef.GetRefBonePose();
	}
	for (int32 i = 0; i < NumMeshBones; ++i)
	{
		if (SkeletonLocalPose.IsValidIndex(MeshToSkeleton[i]))
		{
			BoneSpaceTransforms[i] = SkeletonLocalPose[MeshToSkeleton[i]];
		}
	}
	RefreshBoneTransforms(nullptr);
}
