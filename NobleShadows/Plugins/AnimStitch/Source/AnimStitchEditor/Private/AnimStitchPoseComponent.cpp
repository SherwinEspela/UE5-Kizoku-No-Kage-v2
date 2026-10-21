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
	UpdateBounds();
	MarkRenderTransformDirty(); // sends the new bounds to the renderer; without it the proxy keeps culling against the old ones
}

FBoxSphereBounds UAnimStitchPoseComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const TArray<FTransform>& Bones = GetComponentSpaceTransforms();
	if (Bones.Num() == 0)
	{
		return Super::CalcBounds(LocalToWorld);
	}
	FBox Box(ForceInit);
	for (const FTransform& Bone : Bones)
	{
		Box += Bone.GetTranslation();
	}
	// Skin extends past the joints: pad by the mesh's own size (never less than half a metre).
	const USkinnedAsset* Asset = GetSkinnedAsset();
	const double Pad = FMath::Max(50.0, Asset ? Asset->GetBounds().SphereRadius * 0.5 : 0.0);
	return FBoxSphereBounds(Box.ExpandBy(Pad).TransformBy(LocalToWorld));
}
