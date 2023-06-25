// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/SCompoundWidget.h"
#include "Abilities/GameplayAbility.h"

class FGASTriggerAssetItem : public TSharedFromThis<FGASTriggerAssetItem>
{
public:
	explicit FGASTriggerAssetItem(const FAssetData& Asset, const FAbilityTriggerData& TriggerData);

	FText GetTagName() const;
	FString GetTagNameString() const;
	FText GetTriggerSourceName() const;
	bool HasAssetData() const;
	FText GetAssetName() const;
	FString GetWidgetAssetData() const;

private:
	FAssetData Asset;
	FAbilityTriggerData TriggerData;
};

class SGASTriggerTreeItem : public SMultiColumnTableRow<TSharedPtr<FGASTriggerAssetItem>>
{
public:
	SLATE_BEGIN_ARGS(SGASTriggerTreeItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASTriggerAssetItem>, Item);
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);

private:
	//~ Begin SMultiColumnTableRow Interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	//~ End SMultiColumnTableRow Interface

	void OnAssetNavigate() const;

private:
	TWeakPtr<FGASTriggerAssetItem> WeakItem;
};

class SGASTriggerViewItem : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTriggerDeleted, FGameplayTag);

	SLATE_BEGIN_ARGS(SGASTriggerViewItem)
	{}
		SLATE_ARGUMENT(FGameplayTag, TriggerTag);
		SLATE_EVENT(FOnTriggerDeleted, OnTagDeleted);
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText GetTagToolTip() const;
	FReply OnDelete() const;

private:
	FGameplayTag TriggerTag;
	FOnTriggerDeleted OnDeleteDelegate;
};

#endif