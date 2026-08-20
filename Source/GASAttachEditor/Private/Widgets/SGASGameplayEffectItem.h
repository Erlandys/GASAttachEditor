// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GASAttachEditorAbilityAccessors.h"
#include "Widgets/SGASGameplayEffectsTab.h"

class UAbilitySystemComponent;
struct FModifierSpec;
struct FActiveGameplayEffect;
struct FGameplayModifierInfo;

class FGASGameplayEffectNodeBase : public TSharedFromThis<FGASGameplayEffectNodeBase>
{
public:
	FGASGameplayEffectNodeBase() = default;
	virtual ~FGASGameplayEffectNodeBase() = default;

	void Update();

public:
	FORCEINLINE FText GetName() const { return Name; }
	FORCEINLINE FText GetDurationText() const { return DurationText; }
	FORCEINLINE FText GetStackText() const { return StackText; }
	FORCEINLINE FText GetLevelText() const { return LevelText; }
	FORCEINLINE FText GetPrediction() const { return Prediction; }
	FORCEINLINE FText GetGrantedTags() const { return GrantedTagsText; }
	FORCEINLINE FLinearColor GetColor() const { return Tint; }
	FORCEINLINE FText GetState() const { return StateText; }
	FORCEINLINE EGameplayEffectStateType::Type GetStateType() const { return StateType; }

	bool CanNavigateToSource() const { return SourceAsset.CanNavigate(); }
	void NavigateToSource() const { SourceAsset.Navigate(); }

protected:
	virtual FText GatherName() const { return {}; }
	virtual FText GatherDuration() const { return {}; }
	virtual FText GatherStack() const { return {}; }
	virtual FText GatherLevel() const { return {}; }
	virtual FText GatherPrediction() const { return {}; }
	virtual FText GatherGrantedTags() const { return {}; }
	virtual FText GatherState() const { return {}; }
	virtual bool GatherBlocked() const { return false; }
	virtual EGameplayEffectStateType::Type GatherStateType() const { return EGameplayEffectStateType::Active; }
	// Modifier rows have no asset of their own; only the effect itself overrides this
	virtual const UClass* GatherSourceAssetClass() const { return nullptr; }
	virtual void CreateChildren() {}

public:
	const TArray<TSharedPtr<FGASGameplayEffectNodeBase>>& GetChildNodes() const;

private:
	FText Name;
	FText DurationText;
	FText StackText;
	FText LevelText;
	FText Prediction;
	FText GrantedTagsText;
	FText StateText;
	FLinearColor Tint;
	bool bIsBlocked = false;
	EGameplayEffectStateType::Type StateType = EGameplayEffectStateType::Active;

	// Resolved once and kept, so the source link still works after PIE ends
	FGASSourceAsset SourceAsset;

	void FixupColor();

protected:
	TArray<TSharedPtr<FGASGameplayEffectNodeBase>> ChildNodes;
};

class FGASGameplayEffectNode : public FGASGameplayEffectNodeBase
{
public:
	explicit FGASGameplayEffectNode(const FName WorldContextHandle, const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FActiveGameplayEffectHandle& GameplayEffect);

protected:
	virtual FText GatherName() const override;
	virtual FText GatherDuration() const override;
	virtual FText GatherStack() const override;
	virtual FText GatherLevel() const override;
	virtual FText GatherPrediction() const override;
	virtual FText GatherGrantedTags() const override;
	virtual FText GatherState() const override;
	virtual bool GatherBlocked() const override;
	virtual EGameplayEffectStateType::Type GatherStateType() const override;
	virtual const UClass* GatherSourceAssetClass() const override;
	virtual void CreateChildren() override;

private:
	UWorld* GetWorld() const;
	const FActiveGameplayEffect* GetGameplayEffect() const;

private:
	const FName WorldContextHandle;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	const FActiveGameplayEffectHandle GameplayEffectHandle;

	TMap<int32, TSharedPtr<FGASGameplayEffectNodeBase>> MappedModifiers;
};

class FGASGameplayEffectModifierNode : public FGASGameplayEffectNodeBase
{
public:
	explicit FGASGameplayEffectModifierNode(const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FActiveGameplayEffectHandle& GameplayEffectHandle, int32 ModifierIndex);

protected:
	virtual FText GatherName() const override;
	virtual FText GatherDuration() const override;

private:
	const FActiveGameplayEffect* GetGameplayEffect() const;
	const FModifierSpec* GetModifierSpec(const FActiveGameplayEffect* GameplayEffect) const;
	const FGameplayModifierInfo* GetModifierInfo(const FActiveGameplayEffect* GameplayActiveEffect) const;

private:
	const TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	const FActiveGameplayEffectHandle GameplayEffectHandle;
	const int32 ModifierIndex;
};

class SGASGameplayEffectTreeItem : public SMultiColumnTableRow<TSharedPtr<FGASGameplayEffectNodeBase>>
{
public:
	SLATE_BEGIN_ARGS(SGASGameplayEffectTreeItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASGameplayEffectNodeBase>, WidgetInfoToVisualize)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	//~ Begin SMultiColumnTableRow Interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	//~ End SMultiColumnTableRow Interface

private:
	TSharedRef<SWidget> CreateNameColumn();
	TSharedRef<SWidget> CreateTextColumn(
		const TAttribute<FText>& Text,
		EHorizontalAlignment HorizontalAlignment,
		ETextJustify::Type Justification,
		bool bShowToolTip = false) const;

	void HandleHyperlinkNavigate() const;

private:
	TSharedPtr<FGASGameplayEffectNodeBase> WidgetInfo;
	TAttribute<FText> HighlightText;
};