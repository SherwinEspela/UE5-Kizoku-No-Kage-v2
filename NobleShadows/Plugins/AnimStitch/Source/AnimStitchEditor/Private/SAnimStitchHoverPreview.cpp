#include "SAnimStitchHoverPreview.h"
#include "AnimStitchPreview.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchHoverPreview::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Sequence);
	Collector.AddReferencedObject(Mesh);
}

void SAnimStitchHoverPreview::Construct(const FArguments& InArgs)
{
	Sequence = InArgs._Sequence;
	Mesh = InArgs._Mesh;

	FText Info;
	if (Sequence)
	{
		if (!Mesh || Mesh->GetSkeleton() != Sequence->GetSkeleton())
		{
			Mesh = Sequence->GetSkeleton() ? Sequence->GetSkeleton()->GetPreviewMesh(true) : nullptr;
		}

		const IAnimationDataModel* Model = Sequence->GetDataModel();
		int64 FileSize = 0;
		FString Filename;
		if (FPackageName::DoesPackageExist(InArgs._PackageName, &Filename))
		{
			FileSize = IFileManager::Get().FileSize(*Filename);
		}
		static const FNumberFormattingOptions OneDecimal = FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
		Info = FText::Format(LOCTEXT("HoverInfo",
			"{0}\n{1}\nLength: {2} s  |  {3} frames @ {4} fps\nRoot motion: {5}  |  Notifies: {6}\nSkeleton: {7}\nFile size: {8} KB"),
			FText::FromString(Sequence->GetName()),
			FText::FromString(InArgs._PackageName),
			FText::AsNumber(Sequence->GetPlayLength(), &OneDecimal),
			Model ? Model->GetNumberOfFrames() : 0,
			Model ? FText::AsNumber(Model->GetFrameRate().AsDecimal(), &OneDecimal) : FText::GetEmpty(),
			Sequence->bEnableRootMotion ? LOCTEXT("Yes", "yes") : LOCTEXT("No", "no"),
			Sequence->Notifies.Num(),
			FText::FromString(GetNameSafe(Sequence->GetSkeleton())),
			FText::AsNumber(FileSize / 1024));
	}

	TSharedRef<SWidget> ViewportWidget = SNullWidget::NullWidget;
	if (Sequence && Mesh)
	{
		Preview = MakeShared<FAnimStitchPreview>();
		Preview->SetPinRootInPlace(true);
		Preview->SetShowGrid(true);
		Preview->SetMesh(Mesh);

		FAnimStitchTimeline Timeline;
		FAnimStitchClip Clip;
		Clip.Sequence = Sequence;
		Clip.TrimIn = 0.0;
		Clip.TrimOut = Sequence->GetPlayLength();
		Timeline.Clips.Add(Clip);
		Timeline.Relayout();
		Preview->SetTimeline(Timeline);
		Preview->SetPlayhead(0.0);

		SAssignNew(Viewport, SAnimStitchViewport).Preview(Preview);
		Viewport->FocusOnMesh();
		ViewportWidget = Viewport.ToSharedRef();
		RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SAnimStitchHoverPreview::Advance));
	}
	else
	{
		Info = FText::Format(LOCTEXT("HoverNoMesh", "{0}\n\nNo preview mesh is available for this skeleton."), Info);
	}

	ChildSlot
	[
		SNew(SBox).WidthOverride(320.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(240.f).Visibility(Viewport.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)
				[
					ViewportWidget
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
			[
				SNew(STextBlock).Text(Info).AutoWrapText(true)
			]
		]
	];
}

EActiveTimerReturnType SAnimStitchHoverPreview::Advance(double CurrentTime, float DeltaTime)
{
	const double Length = Sequence ? Sequence->GetPlayLength() : 0.0;
	if (Length > KINDA_SMALL_NUMBER)
	{
		Time = FMath::Fmod(Time + DeltaTime, Length);
		Preview->SetPlayhead(Time);
	}
	return EActiveTimerReturnType::Continue;
}

#undef LOCTEXT_NAMESPACE
