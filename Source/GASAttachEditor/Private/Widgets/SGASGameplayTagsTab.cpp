// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASGameplayTagsTab.h"

#include "SGASGameplayTagsItem.h"
#include "AbilitySystemComponent.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "SGASEditor"

void SGASGameplayTagsTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Vertical)
		+ SSplitter::Slot()
		.Value(.7f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(2.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ActorOwnedTags", "Actor Owned Tags"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
				.VAlign(VAlign_Fill)
				.Padding(2.f)
#if WITH_EDITOR
				.OnMouseButtonUp(this, &SGASGameplayTagsTab::OnSelectTags, true)
#endif
				[
					SAssignNew(OwnedTagsBox, SWrapBox)
					.Orientation(Orient_Horizontal)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(5.f))
				]
			]
		]
		+ SSplitter::Slot()
		.Value(.3f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(2.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ActorBlockedTags", "Actor Blocked Tags"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SBorder)
#if WITH_EDITOR
				.OnMouseButtonUp(this, &SGASGameplayTagsTab::OnSelectTags, false)
#endif
				.VAlign(VAlign_Fill)
				.Padding(2.f)
				[
					SAssignNew(BlockedTagsBox, SWrapBox)
					.Orientation(Orient_Horizontal)
					.UseAllottedSize(true)
					.InnerSlotPadding(FVector2D(5.f))
				]
			]
		]
	];

#if WITH_EDITOR
	EditableOwnedContainers.Empty();
	EditableBlockedContainers.Empty();

	EditableOwnedContainers.Add(SGameplayTagWidget::FEditableGameplayTagContainerDatum(nullptr, &OwnedTagsContainer));
	EditableBlockedContainers.Add(SGameplayTagWidget::FEditableGameplayTagContainerDatum(nullptr, &BlockedTagsContainer));
#endif
}

void SGASGameplayTagsTab::Refresh(UAbilitySystemComponent* Component)
{
	class UDummyAbility : UGameplayAbility
	{
		friend class SGASGameplayTagsTab;
	};

	static FName OwnedTagsProperty = GET_MEMBER_NAME_CHECKED(UDummyAbility, ActivationOwnedTags);
	static FName BlockedTagsProperty = GET_MEMBER_NAME_CHECKED(UDummyAbility, ActivationBlockedTags);

	WeakComponent = Component;

	const auto FillTags = [Component](const FGameplayTagContainer& Tags, FGameplayTagContainer& CurrentTags, const TSharedPtr<SWrapBox>& TagsBox, FGameplayTagContainer& TagsContainer, const bool bOwnedTags)
	{
		if (CurrentTags != Tags)
		{
			CurrentTags = Tags;
			TagsBox->ClearChildren();
			TagsContainer = CurrentTags;

			for (const FGameplayTag& Tag : CurrentTags)
			{
				TSharedRef<FGASTagNode> TagNode = MakeShared<FGASTagNode>(Component, Tag, bOwnedTags ? OwnedTagsProperty : BlockedTagsProperty);
				TagNode->Update();

				TagsBox->AddSlot()
				[
					SNew(SGASTagViewItem)
					.TagNode(TagNode)
				];
			}
		}
	};

	{
		FGameplayTagContainer Tags;
		if (Component)
		{
			Component->GetOwnedGameplayTags(Tags);
		}
		FillTags(Tags, CurrentOwnedTags, OwnedTagsBox, OwnedTagsContainer, true);
	}


	{
		FGameplayTagContainer Tags;
		if (Component)
		{
			Component->GetBlockedAbilityTags(Tags);
		}
		FillTags(Tags, CurrentBlockedTags, BlockedTagsBox, BlockedTagsContainer, false);
	}
}

FReply SGASGameplayTagsTab::OnSelectTags(const FGeometry& Geometry, const FPointerEvent& PointerEvent, const bool bOwnedTags)
{
	if (PointerEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

#if WITH_EDITOR
	const TSharedRef<SGameplayTagWidget> TagWidget =
		SNew(SGameplayTagWidget, bOwnedTags ? EditableOwnedContainers : EditableBlockedContainers)
		.GameplayTagUIMode(EGameplayTagUIMode::SelectionMode)
		.ReadOnly(false)
		.OnTagChanged(this, &SGASGameplayTagsTab::RefreshTagList, bOwnedTags);
#endif

	if (bOwnedTags)
	{
		OldOwnedTagsContainer = OwnedTagsContainer;
	}
	else
	{
		OldBlockedTagsContainer = BlockedTagsContainer;
	}

#if WITH_EDITOR
	if (const FWidgetPath* EventPath = PointerEvent.GetEventPath())
	{
		FSlateApplication::Get().PushMenu(
			AsShared(),
			*EventPath,
			TagWidget,
			PointerEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	}
#endif

	return FReply::Handled();
}

void SGASGameplayTagsTab::RefreshTagList(const bool bOwnedTags)
{
	UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return;
	}

	FGameplayTagContainer& NewTagContainer = bOwnedTags ? OwnedTagsContainer : BlockedTagsContainer;
	FGameplayTagContainer& OldTagContainer = bOwnedTags ? OldOwnedTagsContainer : OldBlockedTagsContainer;

	FGameplayTagContainer AddedTags;
	for (const FGameplayTag& NewTag : NewTagContainer)
	{
		if (OldTagContainer.HasTag(NewTag))
		{
			continue;
		}

		AddedTags.AddTag(NewTag);
	}

	FGameplayTagContainer RemovedTags;
	for (const FGameplayTag& OldTag : OldTagContainer)
	{
		if (NewTagContainer.HasTag(OldTag))
		{
			continue;
		}

		RemovedTags.AddTag(OldTag);
	}

	if (AddedTags.Num() > 0)
	{
		if (bOwnedTags)
		{
			for (const FGameplayTag& AddedTag : AddedTags)
			{
				Component->SetLooseGameplayTagCount(AddedTag, 1);
			}
		}
		else
		{
			Component->BlockAbilitiesWithTags(AddedTags);
		}
	}

	if (RemovedTags.Num() > 0)
	{
		if (bOwnedTags)
		{
			for (const FGameplayTag& RemovedTag : RemovedTags)
			{
				Component->SetLooseGameplayTagCount(RemovedTag, 0);
			}
		}
		else
		{
			Component->UnBlockAbilitiesWithTags(RemovedTags);
		}
	}
}

#undef LOCTEXT_NAMESPACE