// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/SCompoundWidget.h"

class FGASAbilityNode;
class UAbilitySystemComponent;

using SAbilitiesTree = STreeView<TSharedPtr<FGASAbilityNode>>;

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
	TSharedRef<SCheckBox> CreateStateSettingsCheckBox(EAbilityStateType::Type StateType);

	void SortAbilities();

private:
	TSharedPtr<SAbilitiesTree> AbilitiesTree;

	TArray<TSharedPtr<FGASAbilityNode>> AbilitiesList;
	TMap<FGameplayAbilitySpecHandle, TSharedPtr<FGASAbilityNode>> MappedAbilities;
	uint8 VisibleStateTypes = EAbilityStateType::MAX;
	EColumnSortMode::Type SortMode = EColumnSortMode::None;

public:
	static const FName AbilityNameColumn;
	static const FName AbilityStateColumn;
	static const FName AbilityActiveStateColumn;
	static const FName AbilityTriggersColumn;
};
