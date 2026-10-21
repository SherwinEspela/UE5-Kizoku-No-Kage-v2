#include "AnimStitchContentBrowserBadge.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetData.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Types/ISlateMetaData.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

FDelegateHandle FAnimStitchContentBrowserBadge::GeneratorHandle;
TMap<FSoftObjectPath, int32> FAnimStitchContentBrowserBadge::ClipUsage;

void FAnimStitchContentBrowserBadge::SetClipUsage(const TMap<FSoftObjectPath, int32>& Usage) { ClipUsage = Usage; }
void FAnimStitchContentBrowserBadge::ClearClipUsage() { ClipUsage.Reset(); }

const FName FAnimStitchContentBrowserBadge::ScopeTag(TEXT("AnimStitchAssetLibraryBrowser"));

int32 FAnimStitchContentBrowserBadge::GetClipUsage(const FSoftObjectPath& Path)
{
	const int32* Count = ClipUsage.Find(Path);
	return Count ? *Count : 0;
}

namespace
{
	/** Per-badge cache of "is this badge inside the AnimStitch Asset Library browser". */
	struct FBadgeScope
	{
		TWeakPtr<SWidget> Self;
		bool bInside = false;
		double CheckedAt = -1.0;
	};

	/**
	 * The extra-state extender is global to every asset view, so the badge decides for itself where it lives: it is shown only when one of its
	 * ancestors carries the Asset Library's scope tag. Elsewhere (the editor's own Content Browser, pickers) it stays collapsed.
	 */
	bool IsInsideTool(FBadgeScope& Scope)
	{
		const double Now = FPlatformTime::Seconds();
		if (Scope.bInside || (Scope.CheckedAt >= 0.0 && Now - Scope.CheckedAt < 1.0)) { return Scope.bInside; }
		const TSharedPtr<SWidget> Self = Scope.Self.Pin();
		if (!Self.IsValid()) { return false; }
		Scope.CheckedAt = Now;
		for (TSharedPtr<SWidget> Parent = Self->GetParentWidget(); Parent.IsValid(); Parent = Parent->GetParentWidget())
		{
			const TSharedPtr<FTagMetaData> Tag = Parent->GetMetaData<FTagMetaData>();
			if (Tag.IsValid() && Tag->Tag == FAnimStitchContentBrowserBadge::ScopeTag)
			{
				Scope.bInside = true;
				break;
			}
		}
		return Scope.bInside;
	}
}
namespace
{
	/** The badge for one asset: always created for animation sequences, shown only while the sequence is on the timeline. */
	TSharedRef<SWidget> GenerateBadge(const FAssetData& AssetData)
	{
		if (!AssetData.IsInstanceOf(UAnimSequence::StaticClass()))
		{
			return SNullWidget::NullWidget;
		}
		const FSoftObjectPath Path = AssetData.GetSoftObjectPath();
		const TSharedRef<FBadgeScope> Scope = MakeShared<FBadgeScope>();
		TSharedPtr<SBox> Badge;
		SAssignNew(Badge, SBox)
			.Visibility_Lambda([Path, Scope]() { return FAnimStitchContentBrowserBadge::GetClipUsage(Path) > 0 && IsInsideTool(*Scope) ? EVisibility::Visible : EVisibility::Collapsed; })
			.ToolTipText(LOCTEXT("InTimelineTip", "On the AnimStitch timeline"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					// Filled accent circle with a white check mark.
					SNew(SBox).WidthOverride(18.f).HeightOverride(18.f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage).Image(FAppStyle::GetBrush("Icons.FilledCircle")).ColorAndOpacity(FStyleColors::Primary)
						]
						+ SOverlay::Slot().Padding(3.f)
						[
							SNew(SImage).Image(FAppStyle::GetBrush("Icons.Check")).ColorAndOpacity(FLinearColor::White)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(3.f, 0.f, 0.f, 0.f)
				[
					// A count when the same clip is used more than once.
					SNew(STextBlock).ColorAndOpacity(FLinearColor::White).ShadowOffset(FVector2D(1.0, 1.0)).ShadowColorAndOpacity(FLinearColor::Black)
					.Visibility_Lambda([Path]() { return FAnimStitchContentBrowserBadge::GetClipUsage(Path) > 1 ? EVisibility::Visible : EVisibility::Collapsed; })
					.Text_Lambda([Path]() { return FText::FromString(FString::Printf(TEXT("×%d"), FAnimStitchContentBrowserBadge::GetClipUsage(Path))); })
				]
			];
		Scope->Self = Badge;
		return Badge.ToSharedRef();
	}
}

void FAnimStitchContentBrowserBadge::Register()
{
	if (GeneratorHandle.IsValid()) { return; }
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	GeneratorHandle = ContentBrowser.AddAssetViewExtraStateGenerator(FAssetViewExtraStateGenerator(
		FOnGenerateAssetViewExtraStateIndicators::CreateStatic(&GenerateBadge),
		FOnGenerateAssetViewExtraStateIndicators()));
}

void FAnimStitchContentBrowserBadge::Unregister()
{
	if (!GeneratorHandle.IsValid()) { return; }
	if (FContentBrowserModule* ContentBrowser = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		ContentBrowser->RemoveAssetViewExtraStateGenerator(GeneratorHandle);
	}
	GeneratorHandle.Reset();
}

#undef LOCTEXT_NAMESPACE
