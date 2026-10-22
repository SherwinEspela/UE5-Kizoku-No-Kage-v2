#include "SAnimStitchAttachDialog.h"
#include "AnimStitchTimelineAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "ReferenceSkeleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchAttachDialog::Show(UAnimStitchTimelineAsset* Timeline, USkeletalMesh* PreviewMesh)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("AttachTitle", "AnimStitch - Attach Preview Asset"))
		.ClientSize(FVector2D(520.0, 360.0))
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	TSharedRef<SAnimStitchAttachDialog> Dialog = SNew(SAnimStitchAttachDialog).Timeline(Timeline).PreviewMesh(PreviewMesh);
	Dialog->Window = Window;
	Window->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchAttachDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Timeline);
	Collector.AddReferencedObject(PreviewMesh);
}

void SAnimStitchAttachDialog::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
	PreviewMesh = InArgs._PreviewMesh;

	// Sockets first, then every bone.
	if (PreviewMesh)
	{
		for (const USkeletalMeshSocket* Socket : PreviewMesh->GetActiveSocketList())
		{
			if (Socket) { SocketItems.Add(MakeShared<FString>(Socket->SocketName.ToString())); }
		}
		const FReferenceSkeleton& Ref = PreviewMesh->GetRefSkeleton();
		for (int32 b = 0; b < Ref.GetNum(); ++b)
		{
			SocketItems.Add(MakeShared<FString>(Ref.GetBoneName(b).ToString()));
		}
	}

	auto Spin = [this](float& Value, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 3, 0)[SNew(STextBlock).Text(Label)]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 8, 0)
			[
				SNew(SSpinBox<float>).ToolTipText(FText::Format(LOCTEXT("AttachScaleTip", "{0} of the prop (1 = original size)"), Label)).Value_Lambda([&Value]() { return Value; }).OnValueChanged_Lambda([&Value](float V) { Value = V; }).OnValueCommitted_Lambda([&Value](float V, ETextCommit::Type) { Value = V; })
			];
	};
	auto SpinD = [](double& Value, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 3, 0)[SNew(STextBlock).Text(Label)]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 8, 0)
			[
				SNew(SSpinBox<double>).ToolTipText(FText::Format(LOCTEXT("AttachOffsetTip", "{0} offset of the prop from its socket or bone (cm / degrees)"), Label)).Value_Lambda([&Value]() { return Value; }).OnValueChanged_Lambda([&Value](double V) { Value = V; }).OnValueCommitted_Lambda([&Value](double V, ETextCommit::Type) { Value = V; })
			];
	};

	ChildSlot
	[
		SNew(SBorder).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[SNew(STextBlock).Text(LOCTEXT("AttachAsset", "Static or skeletal mesh")).Font(FAppStyle::GetFontStyle("BoldFont"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SObjectPropertyEntryBox)
				.ToolTipText(LOCTEXT("AttachAssetTip", "The static or skeletal mesh to show on the preview character. It is preview only and never baked."))
				.AllowedClass(UObject::StaticClass())
				.ObjectPath_Lambda([this]() { return AssetPath; })
				.OnObjectChanged_Lambda([this](const FAssetData& Data) { AssetPath = Data.GetObjectPathString(); })
				.OnShouldFilterAsset_Lambda([](const FAssetData& Data)
				{
					const UClass* Class = Data.GetClass();
					return !(Class && (Class->IsChildOf(UStaticMesh::StaticClass()) || Class->IsChildOf(USkeletalMesh::StaticClass())));
				})
				.DisplayThumbnail(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[SNew(STextBlock).Text(LOCTEXT("AttachSocket", "Socket (or bone)")).Font(FAppStyle::GetFontStyle("BoldFont"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SSearchableComboBox)
				.ToolTipText(LOCTEXT("AttachSocketTip", "The socket (or any bone) of the preview mesh the prop is attached to"))
				.OptionsSource(&SocketItems)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type) { SelectedSocket = Item; })
				[
					SNew(STextBlock).Text_Lambda([this]() { return SelectedSocket.IsValid() ? FText::FromString(*SelectedSocket) : LOCTEXT("PickSocket", "Pick a socket or bone..."); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[SNew(STextBlock).Text(LOCTEXT("AttachOffset", "Offset from the socket (cm / degrees)")).Font(FAppStyle::GetFontStyle("BoldFont"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Location.X, LOCTEXT("LocX", "X"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Location.Y, LOCTEXT("LocY", "Y"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Location.Z, LOCTEXT("LocZ", "Z"))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Rotation.Roll, LOCTEXT("RotR", "Roll"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Rotation.Pitch, LOCTEXT("RotP", "Pitch"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)[SpinD(Rotation.Yaw, LOCTEXT("RotY", "Yaw"))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SBox).WidthOverride(180.f)[Spin(Scale, LOCTEXT("ScaleLabel", "Scale"))]
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[SNullWidget::NullWidget]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton).Text(LOCTEXT("AttachButton", "Attach")).ToolTipText(LOCTEXT("AttachButtonTip", "Attach the prop to the preview character (needs a mesh and a socket)"))
					.IsEnabled_Lambda([this]() { return !AssetPath.IsEmpty() && SelectedSocket.IsValid(); })
					.OnClicked(this, &SAnimStitchAttachDialog::OnAttach)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("AttachClose", "Close")).ToolTipText(LOCTEXT("AttachCloseTip", "Close without attaching"))
					.OnClicked_Lambda([this]() { if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); } return FReply::Handled(); })
				]
			]
		]
	];
}

FReply SAnimStitchAttachDialog::OnAttach()
{
	FAnimStitchPreviewAttachment Attachment;
	Attachment.Asset = TSoftObjectPtr<UObject>(FSoftObjectPath(AssetPath));
	Attachment.Socket = FName(**SelectedSocket);
	Attachment.Location = Location;
	Attachment.Rotation = Rotation;
	Attachment.Scale = FVector(Scale);
	{
		const FScopedTransaction Tx(LOCTEXT("AttachTx", "Attach Preview Asset"));
		Timeline->AddPreviewAttachment(Attachment);
	}
	if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); }
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
