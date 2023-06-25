// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SGameplayTagWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Views/STreeView.h"

class SGASTriggerViewItem;
class SWrapBox;
class FGASTriggerAssetItem;

class SGASTriggersWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASTriggersWidget)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnMoueButtonUp(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void OnTagChanged();
	void OnTagDeleted(FGameplayTag TriggerTag);
	void UpdateAssetsList();

private:
	TSharedPtr<SWrapBox> TriggerTagsView;

	using STriggerAssetsTree = STreeView<TSharedPtr<FGASTriggerAssetItem>>;
	TSharedPtr<STriggerAssetsTree> TriggerAssetsTree;
	TArray<TSharedPtr<FGASTriggerAssetItem>> TriggerAssetsTreeList;

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