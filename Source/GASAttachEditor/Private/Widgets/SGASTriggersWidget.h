// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "SGameplayTagWidget.h"
#include "GameplayTagContainer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Views/SListView.h"

class SWrapBox;
class SGASTriggerViewItem;
class FGASTriggerAssetItem;

class SGASTriggersWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASTriggersWidget)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleTriggerTagsMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void OnTagChanged();
	void OnTagDeleted(FGameplayTag TriggerTag);
	void UpdateAssetsList();

private:
	TSharedPtr<SWrapBox> TriggerTagsView;

	// Flat list - trigger assets have no children
	using STriggerAssetsList = SListView<TSharedPtr<FGASTriggerAssetItem>>;
	TSharedPtr<STriggerAssetsList> TriggerAssetsList;
	TArray<TSharedPtr<FGASTriggerAssetItem>> TriggerAssetsListItems;

	TArray<SGameplayTagWidget::FEditableGameplayTagContainerDatum> EditableTriggersContainer;
	FGameplayTagContainer TriggersContainer;
	FGameplayTagContainer OldTriggersContainer;

	TMap<FGameplayTag, TSharedPtr<SGASTriggerViewItem>> TriggersList;

public:
	static const FName TriggerTagColumn;
	static const FName TriggerAssetColumn;
	static const FName TriggerSourceColumn;
};

#endif