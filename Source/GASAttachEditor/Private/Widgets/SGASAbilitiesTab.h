// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "GameplayAbilitySpecHandle.h"
#include "Misc/TextFilter.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/SHeaderRow.h"

class SCheckBox;
class SSearchBox;
class FGASAbilityNode;
class UAbilitySystemComponent;

using SAbilitiesTree = STreeView<TSharedPtr<FGASAbilityNode>>;
using FGASAbilityTextFilter = TTextFilter<const FGASAbilityNode&>;

namespace EAbilityStateType
{
	enum Type
	{
		Active		= 1 << 0,
		Blocked		= 1 << 1,
		Inactive	= 1 << 2,
		MAX			= 0xFF
	};
};

class SGASAbilitiesTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASAbilitiesTab)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

public:
	void Refresh(UAbilitySystemComponent* Component);

private:
	TSharedRef<SWidget> CreateSearchBox();
	TSharedRef<SCheckBox> CreateStateSettingsCheckBox(EAbilityStateType::Type StateType);

	void SortAbilities();

private:
	void SaveHiddenColumns();

	void LoadSettings();
	void SaveSettings() const;

	static const TCHAR* VisibleStatesKey;
	static const TCHAR* SortModeKey;
	static const TCHAR* HiddenColumnsKey;

	void PopulateSearchStrings(const FGASAbilityNode& Node, TArray<FString>& OutSearchStrings) const;
	FText GetHighlightText() const;

	bool IsFilterActive() const;
	bool MatchesFilter(const FGASAbilityNode& Node) const;
	bool PassesFilter(const TSharedPtr<FGASAbilityNode>& Node) const;
	void ApplyFilter();

private:
	TSharedPtr<SAbilitiesTree> AbilitiesTree;
	TSharedPtr<SHeaderRow> HeaderRow;
	TArray<FName> HiddenColumns;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<FGASAbilityTextFilter> SearchFilter;

	TArray<TSharedPtr<FGASAbilityNode>> AbilitiesList;
	TArray<TSharedPtr<FGASAbilityNode>> FilteredAbilitiesList;
	TMap<FGameplayAbilitySpecHandle, TSharedPtr<FGASAbilityNode>> MappedAbilities;
	uint8 VisibleStateTypes = EAbilityStateType::MAX;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

public:
	static const FName AbilityNameColumn;
	static const FName AbilityStateColumn;
	static const FName AbilityActiveStateColumn;
	static const FName AbilityTriggersColumn;
};