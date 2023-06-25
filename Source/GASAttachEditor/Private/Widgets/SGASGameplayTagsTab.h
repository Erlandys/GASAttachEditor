// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "SGameplayTagWidget.h"
#endif
#include "Widgets/SCompoundWidget.h"

class UAbilitySystemComponent;
class SWrapBox;

class GASATTACHEDITOR_API SGASGameplayTagsTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASGameplayTagsTab)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Refresh(UAbilitySystemComponent* Component);

private:
	FReply OnSelectTags(const FGeometry& Geometry, const FPointerEvent& PointerEvent, bool bOwnedTags);
	void RefreshTagList(bool bOwnedTags);

private:
	TSharedPtr<SWrapBox> OwnedTagsBox;
	TSharedPtr<SWrapBox> BlockedTagsBox;

private:
	FGameplayTagContainer CurrentOwnedTags;
	FGameplayTagContainer CurrentBlockedTags;

#if WITH_EDITOR
	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableOwnedContainers;
	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableBlockedContainers;
#endif

	FGameplayTagContainer OwnedTagsContainer;
	FGameplayTagContainer OldOwnedTagsContainer;

	FGameplayTagContainer BlockedTagsContainer;
	FGameplayTagContainer OldBlockedTagsContainer;

	TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
};
