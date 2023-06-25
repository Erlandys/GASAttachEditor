// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_EDITOR

#include "SGASTriggerTreeItem.h"

#include "SGASTriggersWidget.h"

#include "EditorFontGlyphs.h"
#include "GameplayTagsManager.h"
#include "Widgets/Input/SHyperlink.h"
#include "AssetRegistry/AssetRegistryModule.h"

FGASTriggerAssetItem::FGASTriggerAssetItem(const FAssetData& Asset, const FAbilityTriggerData& TriggerData)
	: Asset(Asset)
	, TriggerData(TriggerData)
{
}

FText FGASTriggerAssetItem::GetTagName() const
{
	return FText::FromName(TriggerData.TriggerTag.GetTagName());
}

FString FGASTriggerAssetItem::GetTagNameString() const
{
	return TriggerData.TriggerTag.GetTagName().ToString();
}

FText FGASTriggerAssetItem::GetTriggerSourceName() const
{
	return UEnum::GetDisplayValueAsText(TriggerData.TriggerSource);
}

bool FGASTriggerAssetItem::HasAssetData() const
{
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
	{
		return true;
	}

	return !Asset.GetClass()->IsNative();
}

FText FGASTriggerAssetItem::GetAssetName() const
{
	return FText::FromName(Asset.AssetName);
}

FString FGASTriggerAssetItem::GetWidgetAssetData() const
{
	return FSoftObjectPath(Asset.GetAsset()).ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASTriggerTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	WeakItem = InArgs._Item;
	SMultiColumnTableRow<TSharedPtr<FGASTriggerAssetItem>>::Construct(SMultiColumnTableRow<TSharedPtr<FGASTriggerAssetItem>>::FArguments(), OwnerTable);
}

TSharedRef<SWidget> SGASTriggerTreeItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	const TSharedPtr<FGASTriggerAssetItem> Item = WeakItem.Pin();
	if (!ensure(Item))
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == SGASTriggersWidget::TriggerTagColumn)
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Item->GetTagName())
				.Justification(ETextJustify::Center)
			];
	}

	if (ColumnName == SGASTriggersWidget::TriggerAssetColumn)
	{
		TSharedPtr<SWidget> TextWidget = nullptr;
		if (Item->HasAssetData())
		{
			TextWidget =
				SNew(SBorder)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f))
				[
					SNew(SHyperlink)
					.Text(Item->GetAssetName())
					.OnNavigate(this, &SGASTriggerTreeItem::OnAssetNavigate)
				];
		}
		else
		{
			TextWidget =
				SNew(STextBlock)
				.Text(Item->GetAssetName());
		}

		return
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				TextWidget.ToSharedRef()
			];
	}

	ensure(ColumnName == SGASTriggersWidget::TriggerSourceColumn);

	return
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Item->GetTriggerSourceName())
		];
}

void SGASTriggerTreeItem::OnAssetNavigate() const
{
	const TSharedPtr<FGASTriggerAssetItem> Item = WeakItem.Pin();
	if (!ensure(Item))
	{
		return;
	}

	if (!ensure(Item->HasAssetData()))
	{
		return;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(*Item->GetWidgetAssetData()));

	if (!AssetData.IsValid())
	{
		return;
	}

	UObject* Object = AssetData.GetAsset();
	if (!ensure(Object))
	{
		return;
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASTriggerViewItem::Construct(const FArguments& InArgs)
{
	TriggerTag = InArgs._TriggerTag;
	OnDeleteDelegate = InArgs._OnTagDeleted;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(1.f)
		.ToolTipText_Raw(this, &SGASTriggerViewItem::GetTagToolTip)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(0.f)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Danger")
				.ForegroundColor(FSlateColor::UseForeground())
				.OnClicked(this, &SGASTriggerViewItem::OnDelete)
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
					.Text(FEditorFontGlyphs::Times)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TriggerTag.ToString()))
			]
		]
	];
}

FText SGASTriggerViewItem::GetTagToolTip() const
{
	FString Comment;
	FName TagSource;
	bool bIsTagExplicit;
	bool bIsRestrictedTag;
	bool bAllowNonRestrictedChildren;

	if (UGameplayTagsManager::Get().GetTagEditorData(*TriggerTag.ToString(), Comment, TagSource, bIsTagExplicit, bIsRestrictedTag, bAllowNonRestrictedChildren))
	{
		return FText::FromString(Comment);
	}

	return {};
}

FReply SGASTriggerViewItem::OnDelete() const
{
	OnDeleteDelegate.ExecuteIfBound(TriggerTag);
	return FReply::Handled();
}

#endif