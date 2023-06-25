#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Widgets/SGASAbilitiesTab.h"

class UAbilitySystemComponent;
class STableViewBase;

enum class EGAAbilityNode
{
	Ability,
	Task,
};

class FGASAbilityNode : public TSharedFromThis<FGASAbilityNode>
{
public:
	explicit FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpec& AbilitySpec);
	explicit FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpec& AbilitySpec, const TWeakObjectPtr<UGameplayTask>& InGameplayTask);

public:
	void Update(const FGameplayAbilitySpec& AbilitySpec, uint8 VisibleStates);
	void UpdateVisibility(uint8 VisibleStates);

	FORCEINLINE FText GetName() const { return Name; }
	FORCEINLINE FLinearColor GetColor() const { return Tint; }
	FORCEINLINE FText GetState() const { return State; }
	FORCEINLINE FText GetActiveState() const { return ActiveState; }
	FORCEINLINE FText GetTriggersData() const { return TriggersData; }
	FORCEINLINE EGAAbilityNode GetNodeType() const { return Type; }
	FORCEINLINE EVisibility GetVisibility() const { return Visibility; }

private:
	FText FetchName() const;
	FText FetchState(EAbilityStateType::Type &OutStateType) const;
	FText FetchTriggersData() const;
	bool IsActive() const { return WeakComponent.IsValid() && AbilitySpecPtr.IsActive(); }
	void FixupColor();
	void FixupTasks(uint8 VisibleStates);

public:
	bool HasValidWidgetAssetData() const { return Type == EGAAbilityNode::Ability && !GetWidgetAssetData().IsEmpty(); }
	FString GetWidgetAssetData() const;

	const TArray<TSharedPtr<FGASAbilityNode>>& GetChildNodes() const;

private:
	TMap<int32, TSharedPtr<FGASAbilityNode>> MappedChildNodes;
	TArray<TSharedPtr<FGASAbilityNode>> ChildNodes;

	FText Name;
	FLinearColor Tint;
	FText State;
	FText ActiveState;
	FText TriggersData;
	EVisibility Visibility;

	EGAAbilityNode Type = EGAAbilityNode::Ability;

	EAbilityStateType::Type StateType = EAbilityStateType::Active;

	FGameplayAbilitySpec AbilitySpecPtr;
	TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	TWeakObjectPtr<UGameplayTask> GameplayTask;
};

class SGASAbilityItem : public SMultiColumnTableRow<TSharedRef<FGASAbilityNode>>
{
public:

	SLATE_BEGIN_ARGS(SGASAbilityItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASAbilityNode>, WidgetInfoToVisualize)
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
};