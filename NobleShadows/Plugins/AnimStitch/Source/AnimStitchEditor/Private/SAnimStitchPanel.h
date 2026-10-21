#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchTimelineAsset.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/Views/STileView.h"
#include "Framework/Docking/TabManager.h"

class SAnimStitchTimeline;
class SAnimStitchViewport;
class SDockTab;
class FAssetThumbnail;
class FAssetThumbnailPool;
class FAnimStitchPreview;
struct FAssetData;

/** Dockable AnimStitch tab content: preview viewport, transport, timeline, status line. */
class SAnimStitchPanel : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAnimStitchPanel() override;

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchPanel"); }

	/** Builds the dockable layout (toolbar, viewport, timeline, bottom bar, side panel) inside the given tab and restores the saved one. */
	void InitializeDocking(const TSharedRef<SDockTab>& Tab);
	/** Set while a layout reset is in progress: the old layout must not be saved again. */
	static bool bSkipLayoutPersist;

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
	TSharedPtr<class FScopedTransaction> RateSliderTx;
	int32 InspectorTab = 0; // 0 = Details, 1 = Control Properties, 2 = Attached Props
	int32 LibraryTab = 0; // Asset Library: 0 = Content Browser, 1 = Baked Assets
	TSharedRef<SWidget> MakeBakedAssetsTab();
	TSharedRef<SWidget> MakeOpenRecentMenu();
	static void AddRecentTimeline(const class UAnimStitchTimelineAsset* Saved);
	/** One value panel per bone track, created on demand, so any number of tracks can be expanded at once. */
	void SyncKeyPanels();
	TSharedPtr<class SOverlay> KeyPanelOverlay;
	int32 KeyPanelCount = 0;
	void PersistLayoutIfChanged(double Now);
	double LastLayoutCheck = 0.0;
	FString LastLayoutText;
	void SaveLayoutAs();
	void LoadSavedLayout(const FString& Name);
	void DeleteSavedLayout(const FString& Name);
	TSharedRef<FTabManager::FLayout> MakeDefaultLayout() const;
	TSharedRef<SWidget> MakeLayoutMenu();
	void ResetLayout();
	TSharedPtr<SWidget> ViewportSection, ToolbarSection, TimelineSection, InspectorSection, AssetLibrarySection;
	/** Unsaved edits (shown as an asterisk on the Save icon). */
	bool IsDirtyState() const;
	/** Save / discard / cancel prompt before work is thrown away. False = the user cancelled (or a save did not finish). */
	bool ConfirmDiscardChanges();
	int32 LastControlKind = 0;
	int32 LastControlTrack = -1;
	double LastPlantAnalysis = 0.0;
	double LastBrowserPoll = 0.0;
	FString LastBrowserSelection;
	void PublishClipUsage();
	void SyncBrowserSelection(double Now);
	TSharedPtr<FTabManager> TabManager;
	uint32 LastTrackSerial = 0;
	void RebuildBakedList();
	/** One tile of the Baked Assets tab. */
	struct FBakedItem
	{
		FSoftObjectPath Path;
		TSharedPtr<FAssetThumbnail> Thumbnail;
	};
	TArray<TSharedPtr<FBakedItem>> BakedItems;
	TSharedPtr<STileView<TSharedPtr<FBakedItem>>> BakedList;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
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





