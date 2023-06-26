// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

struct FActiveGameplayEffectHandle;
class UAbilitySystemComponent;
class FGASGameplayEffectNodeBase;

using SGameplayEffectsTree = STreeView<TSharedPtr<FGASGameplayEffectNodeBase>>;

class GASATTACHEDITOR_API SGASGameplayEffectsTab : public SCompoundWidget
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
	TSharedPtr<SGameplayEffectsTree> GameplayEffectsTree;

private:
	TArray<TSharedPtr<FGASGameplayEffectNodeBase>> GameplayEffectsList;
	TMap<FActiveGameplayEffectHandle, TSharedPtr<FGASGameplayEffectNodeBase>> MappedGameplayEffects;

public:
	static const FName GameplayEffectNameColumn;
	static const FName GameplayEffectDurationColumn;
	static const FName GameplayEffectStackColumn;
	static const FName GameplayEffectLevelColumn;
	static const FName GameplayEffectPredictionColumn;
	static const FName GameplayEffectGrantedTagsColumn;
};
