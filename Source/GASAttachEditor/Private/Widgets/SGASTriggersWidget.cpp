// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_EDITOR

#include "SGASTriggersWidget.h"

#include "SGASTriggerTreeItem.h"
#include "GASAttachEditorAbilityAccessors.h"

#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Abilities/GameplayAbility.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

const FName SGASTriggersWidget::TriggerTagColumn = "Trigger_Tag";
const FName SGASTriggersWidget::TriggerAssetColumn = "Trigger_Asset";
const FName SGASTriggersWidget::TriggerSourceColumn = "Trigger_Source";

void SGASTriggersWidget::Construct(const FArguments& InArgs)
{
	TriggersContainer.Reset();
	EditableTriggersContainer.Empty();
	EditableTriggersContainer.Add(SGameplayTagWidget::FEditableGameplayTagContainerDatum(nullptr, &TriggersContainer));

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.Value(.2f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(2.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TriggerTags", "Trigger Tags"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
				.Padding(2.f)
				.OnMouseButtonUp(this, &SGASTriggersWidget::HandleTriggerTagsMouseButtonUp)
				[
					SAssignNew(TriggerTagsView, SWrapBox)
					.Orientation(Orient_Horizontal)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(5.f))
				]
			]
		]
		+ SSplitter::Slot()
		.Value(.8f)
		[
			SNew(SBorder)
			.Padding(0.f)
			[
				SAssignNew(TriggerAssetsList, STriggerAssetsList)
				.ListItemsSource(&TriggerAssetsListItems)
				.OnGenerateRow_Lambda([](TSharedPtr<FGASTriggerAssetItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return
						SNew(SGASTriggerTreeItem, OwnerTable)
						.Item(Item);
				})
				.HeaderRow
				(
					SNew(SHeaderRow)
					.CanSelectGeneratedColumn(true)
					+ SHeaderRow::Column(TriggerTagColumn)
					.DefaultLabel(LOCTEXT("TriggerTagColumn", "Tag"))
					.FillWidth(.3f)
					.ShouldGenerateWidget(true)
					+ SHeaderRow::Column(TriggerAssetColumn)
					.DefaultLabel(LOCTEXT("TriggerAssetColumn", "Asset"))
					.FillWidth(.5f)
					+ SHeaderRow::Column(TriggerSourceColumn)
					.DefaultLabel(LOCTEXT("TriggerSourceColumn", "Source"))
					.FillWidth(.2f)
				)
			]
		]
	];
}

FReply SGASTriggersWidget::HandleTriggerTagsMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	if (PointerEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

	const TSharedPtr<SGameplayTagWidget> TagsWidget =
		SNew(SGameplayTagWidget, EditableTriggersContainer)
		.GameplayTagUIMode(EGameplayTagUIMode::SelectionMode)
		.ReadOnly(false)
		.OnTagChanged(this, &SGASTriggersWidget::OnTagChanged);

	const FWidgetPath WidgetPath = PointerEvent.GetEventPath() ? *PointerEvent.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, TagsWidget.ToSharedRef(), PointerEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

	return FReply::Handled();
}

void SGASTriggersWidget::OnTagChanged()
{
	for (const FGameplayTag& NewTag : TriggersContainer)
	{
		if (OldTriggersContainer.HasTag(NewTag))
		{
			continue;
		}

		if (TriggersList.Contains(NewTag))
		{
			continue;
		}

		TSharedRef<SGASTriggerViewItem> TriggerViewItem =
			SNew(SGASTriggerViewItem)
			.TriggerTag(NewTag)
			.OnTagDeleted(this, &SGASTriggersWidget::OnTagDeleted);

		TriggerTagsView->AddSlot()
		[
			TriggerViewItem
		];

		TriggersList.Add(NewTag, TriggerViewItem);
	}

	for (const FGameplayTag& OldTag : OldTriggersContainer)
	{
		if (TriggersContainer.HasTag(OldTag))
		{
			continue;
		}

		TSharedPtr<SGASTriggerViewItem> TriggerViewItem = TriggersList.FindRef(OldTag);
		if (!TriggerViewItem)
		{
			continue;
		}

		TriggerTagsView->RemoveSlot(TriggerViewItem.ToSharedRef());
		TriggersList.Remove(OldTag);
	}

	UpdateAssetsList();
}

void SGASTriggersWidget::OnTagDeleted(const FGameplayTag TriggerTag)
{
	if (TriggersContainer.HasTag(TriggerTag))
	{
		TriggersContainer.RemoveTag(TriggerTag);
		OldTriggersContainer = TriggersContainer;
	}

	if (const TSharedPtr<SGASTriggerViewItem> TriggerViewItem = TriggersList.FindRef(TriggerTag))
	{
		TriggerTagsView->RemoveSlot(TriggerViewItem.ToSharedRef());
		TriggersList.Remove(TriggerTag);
	}

	UpdateAssetsList();
}

void SGASTriggersWidget::UpdateAssetsList()
{
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	TArray<FAssetDependency> Dependencies;
	for (const FGameplayTag& TriggerTag : TriggersContainer)
	{
		FAssetIdentifier AssetId = FAssetIdentifier(FGameplayTag::StaticStruct(), TriggerTag.GetTagName());
		AssetRegistry.GetReferencers(AssetId, Dependencies, UE::AssetRegistry::EDependencyCategory::SearchableName, UE::AssetRegistry::EDependencyQuery::NoRequirements);
	}

	TriggerAssetsListItems.Empty();

	for (const FAssetDependency& Dependency : Dependencies)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(Dependency.AssetId.PackageName, Assets);

		for (const FAssetData& Asset : Assets)
		{
			UObject* Object = Asset.GetAsset();
			const UGameplayAbility* Ability = Cast<UGameplayAbility>(Object);

			if (!Ability)
			{
				if (const UBlueprint* Blueprint = Cast<UBlueprint>(Object))
				{
					if (Blueprint->GeneratedClass &&
						Blueprint->GeneratedClass->IsChildOf<UGameplayAbility>())
					{
						Ability = Blueprint->GeneratedClass->GetDefaultObject<UGameplayAbility>();
					}
				}
			}

			if (!Ability)
			{
				continue;
			}

			const TArray<FAbilityTriggerData>* AbilityTriggersPtr = FGASAbilityAccessors::FindAbilityTriggers(Ability);
			if (!AbilityTriggersPtr)
			{
				continue;
			}

			TArray<FAbilityTriggerData> AbilityTriggers = *AbilityTriggersPtr;
			for (const FAbilityTriggerData& Trigger : AbilityTriggers)
			{
				if (!TriggersContainer.HasTag(Trigger.TriggerTag))
				{
					continue;
				}

				TriggerAssetsListItems.Add(MakeShared<FGASTriggerAssetItem>(Asset, Trigger));
				break;
			}
		}
	}

	TriggerAssetsListItems.Sort([](const TSharedPtr<FGASTriggerAssetItem>& A, const TSharedPtr<FGASTriggerAssetItem>& B)
	{
		return A->GetTagNameString() > B->GetTagNameString();
	});

	TriggerAssetsList->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE

#endif