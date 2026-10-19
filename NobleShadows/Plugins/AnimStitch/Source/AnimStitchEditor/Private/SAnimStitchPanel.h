#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchTimelineAsset.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/Views/SListView.h"

class SAnimStitchTimeline;
class SAnimStitchViewport;
class FAnimStitchPreview;
struct FAssetData;

/** Dockable AnimStitch tab content: preview viewport, transport, timeline, status line. */
class SAnimStitchPanel : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchPanel"); }

	/** Loads another timeline asset into the panel (Open). */
	void SetAsset(UAnimStitchTimelineAsset* NewAsset);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void UpdatePlantLanes(double Now);
	TSharedPtr<struct FAnimStitchFootLockResult> PlantData;
	uint32 PlantCounter = MAX_uint32;
	double PlantDirtyAt = 0.0;
	bool bPlantDirty = false;

	// Viewport "Show" menu (bones, sockets, grid) like the animation editor's
	TSharedRef<SWidget> MakeShowMenu();
	TSharedRef<SWidget> MakeViewportToolbar();
	void FillControlsMenu(class FMenuBuilder& Menu);
	void FillAttachmentsMenu(class FMenuBuilder& Menu);
	void FillBonesMenu(class FMenuBuilder& Menu);
	int32 RightTab = 0; // 0 = Content Browser, 1 = Details, 2 = Baked Assets
	TSharedRef<SWidget> MakeBakedAssetsTab();
	void RebuildBakedList();
	TArray<TSharedPtr<FSoftObjectPath>> BakedItems;
	TSharedPtr<SListView<TSharedPtr<FSoftObjectPath>>> BakedList;
	TSharedPtr<class SScrollBar> TrackScrollBar;

	// Bone dropdown (alternative to clicking a joint in the viewport)
	void RebuildBoneItems();
	void OnBoneComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	void OnPreviewBoneSelected();
	TArray<TSharedPtr<FString>> BoneItems;
	TSharedPtr<class SSearchableComboBox> BoneCombo;
	TWeakObjectPtr<class USkeletalMesh> BoneItemsMesh;

	void OnTimelineScrubbed();
	void OpenFromDisk();
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& Selected);
	void NewTimeline();
	void Save();
	void SaveAs();
	void OpenBatch();
	void OpenBake();
	FText GetAssetLabel() const;
public:
	/** Title for the dock tab: the opened timeline asset name. */
	FText GetTabTitle() const;
private:
	FGetCurrentSelectionDelegate BrowserSelectionDelegate;

	/** Fills the right-hand browser with the animation sequences of the preview mesh's skeleton. */
	void RebuildSequenceBrowser();
	TSharedPtr<class SBox> BrowserBox;
	TWeakObjectPtr<USkeleton> BrowserSkeleton;

	void AutoSelectPreviewMesh();
	void TogglePlayback();
	TWeakObjectPtr<USkeleton> LastSkeleton;

	EActiveTimerReturnType UpdatePreview(double CurrentTime, float DeltaTime);
	void SyncPreviewToTimeline();

	void Play();
	void Pause();
	void Stop();

	FText GetStatusText() const;
	FText GetTimeText() const;
	FText GetProblemText() const;
	EVisibility GetProblemVisibility() const;
	void HandleWarnings(const TArray<FText>& Warnings);
	bool IsCurveEnabled() const;
	ECheckBoxState GetCurveState(EAnimStitchBlendCurve Curve) const;

	FString GetMeshPath() const { return MeshPath; }
	void OnMeshChanged(const FAssetData& Data);
	bool ShouldFilterMesh(const FAssetData& Data) const;

	TObjectPtr<UAnimStitchTimelineAsset> Asset;
	TSharedPtr<SAnimStitchTimeline> Timeline;
	TSharedPtr<SAnimStitchViewport> Viewport;
	TSharedPtr<FAnimStitchPreview> Preview;

	FText LastWarning;
	FString MeshPath;
	uint32 LastEditCounter = MAX_uint32;
	uint32 LastManualCounter = MAX_uint32;
	double LastPlayhead = -1.0;
	bool bPlaying = false;
	bool bLoop = false;
	bool bShowGhosts = false;
	bool bShowRootPath = false;
};





