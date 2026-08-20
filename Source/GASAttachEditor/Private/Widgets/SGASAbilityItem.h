// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTask.h"
#include "GameplayAbilitySpec.h"
#include "GASAttachEditorAbilityAccessors.h"
#include "UObject/ObjectKey.h"
#include "Widgets/SGASAbilitiesTab.h"

class STableViewBase;
class UAbilitySystemComponent;

enum class EGAAbilityNode
{
	Ability,
	Task,
};

class FGASAbilityNode : public TSharedFromThis<FGASAbilityNode>
{
public:
	explicit FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle);
	explicit FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const TWeakObjectPtr<UGameplayTask>& InGameplayTask);

public:
	void Update();

	FORCEINLINE FText GetName() const { return Name; }
	FORCEINLINE FLinearColor GetColor() const { return Tint; }
	FORCEINLINE FText GetState() const { return State; }
	FORCEINLINE FText GetActiveState() const { return ActiveState; }
	FORCEINLINE FText GetTriggersData() const { return TriggersData; }
	FORCEINLINE EGAAbilityNode GetNodeType() const { return Type; }
	FORCEINLINE EAbilityStateType::Type GetStateType() const { return StateType; }

private:
	const FGameplayAbilitySpec* FindAbilitySpec() const;
	UGameplayAbility* FindAbility() const;

	FText FetchName() const;
	FText FetchState(EAbilityStateType::Type& OutStateType) const;
	FText FetchTriggersData() const;
	void FetchSourceAsset();
	bool IsActive() const;
	void FixupColor();
	void FixupTasks();

public:
	bool CanNavigateToSource() const { return SourceAsset.CanNavigate(); }
	void NavigateToSource() const { SourceAsset.Navigate(); }

	const TArray<TSharedPtr<FGASAbilityNode>>& GetChildNodes() const;

private:
	TMap<FObjectKey, TSharedPtr<FGASAbilityNode>> MappedChildNodes;
	TArray<TSharedPtr<FGASAbilityNode>> ChildNodes;

	FText Name;
	FLinearColor Tint = FLinearColor::White;
	FText State;
	FText ActiveState;
	FText TriggersData;

	EGAAbilityNode Type = EGAAbilityNode::Ability;

	EAbilityStateType::Type StateType = EAbilityStateType::Active;

	// Resolved once and kept, so the source link still works after PIE ends
	FGASSourceAsset SourceAsset;

	FGameplayAbilitySpecHandle AbilitySpecHandle;
	TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	TWeakObjectPtr<UGameplayTask> GameplayTask;
};

class SGASAbilityItem : public SMultiColumnTableRow<TSharedRef<FGASAbilityNode>>
{
public:

	SLATE_BEGIN_ARGS(SGASAbilityItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASAbilityNode>, WidgetInfoToVisualize)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	//~ Begin SMultiColumnTableRow Interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	//~ End SMultiColumnTableRow Interface

private:
	TSharedRef<SWidget> CreateNameColumn();
	TSharedRef<SWidget> CreateStateColumn() const;
	TSharedRef<SWidget> CreateActiveStateColumn() const;
	TSharedRef<SWidget> CreateTriggersColumn() const;

	void HandleHyperlinkNavigate() const;

private:
	TSharedPtr<FGASAbilityNode> WidgetInfo;
	TAttribute<FText> HighlightText;
};