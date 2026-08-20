// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ActiveGameplayEffectHandle.h"
#include "Misc/TextFilter.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/SHeaderRow.h"

class SCheckBox;
class SSearchBox;
class UAbilitySystemComponent;
class FGASGameplayEffectNodeBase;

using SGameplayEffectsTree = STreeView<TSharedPtr<FGASGameplayEffectNodeBase>>;
using FGASGameplayEffectTextFilter = TTextFilter<const FGASGameplayEffectNodeBase&>;

namespace EGameplayEffectStateType
{
	enum Type
	{
		Active		= 1 << 0,
		Inhibited	= 1 << 1,
		Infinite	= 1 << 2,
		MAX			= 0xFF
	};
};

class SGASGameplayEffectsTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASGameplayEffectsTab)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

public:
	void Refresh(UAbilitySystemComponent* Component, FName WorldContextHandle);

private:
	TSharedRef<SWidget> CreateSearchBox();
	TSharedRef<SCheckBox> CreateStateSettingsCheckBox(EGameplayEffectStateType::Type StateType);

	void SortGameplayEffects();

private:
	void SaveHiddenColumns();

	void LoadSettings();
	void SaveSettings() const;

	static const TCHAR* VisibleStatesKey;
	static const TCHAR* SortModeKey;
	static const TCHAR* HiddenColumnsKey;

	void PopulateSearchStrings(const FGASGameplayEffectNodeBase& Node, TArray<FString>& OutSearchStrings) const;
	FText GetHighlightText() const;

	bool IsFilterActive() const;
	bool MatchesText(const FGASGameplayEffectNodeBase& Node) const;
	bool PassesTextFilter(const TSharedPtr<FGASGameplayEffectNodeBase>& Node) const;
	void ApplyFilter();

private:
	TSharedPtr<SGameplayEffectsTree> GameplayEffectsTree;
	TSharedPtr<SHeaderRow> HeaderRow;
	TArray<FName> HiddenColumns;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<FGASGameplayEffectTextFilter> SearchFilter;

	uint8 VisibleStateTypes = EGameplayEffectStateType::MAX;
	EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;

private:
	TArray<TSharedPtr<FGASGameplayEffectNodeBase>> GameplayEffectsList;
	TArray<TSharedPtr<FGASGameplayEffectNodeBase>> FilteredGameplayEffectsList;
	TMap<FActiveGameplayEffectHandle, TSharedPtr<FGASGameplayEffectNodeBase>> MappedGameplayEffects;

public:
	static const FName GameplayEffectNameColumn;
	static const FName GameplayEffectStateColumn;
	static const FName GameplayEffectDurationColumn;
	static const FName GameplayEffectStackColumn;
	static const FName GameplayEffectLevelColumn;
	static const FName GameplayEffectPredictionColumn;
	static const FName GameplayEffectGrantedTagsColumn;
};