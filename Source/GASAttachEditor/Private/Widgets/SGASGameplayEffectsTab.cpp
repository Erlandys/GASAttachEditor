// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASGameplayEffectsTab.h"

#include "AbilitySystemComponent.h"
#include "SGASGameplayEffectItem.h"

#define LOCTEXT_NAMESPACE "SGASEditor"

const FName SGASGameplayEffectsTab::GameplayEffectNameColumn = "GameplayEffect_Name";
const FName SGASGameplayEffectsTab::GameplayEffectDurationColumn = "GameplayEffect_Duration";
const FName SGASGameplayEffectsTab::GameplayEffectStackColumn = "GameplayEffect_Stack";
const FName SGASGameplayEffectsTab::GameplayEffectLevelColumn = "GameplayEffect_Level";
const FName SGASGameplayEffectsTab::GameplayEffectPredictionColumn = "GameplayEffect_Prediction";
const FName SGASGameplayEffectsTab::GameplayEffectGrantedTagsColumn = "GameplayEffect_GrantedTags";

void SGASGameplayEffectsTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(0.f)
		[
			SAssignNew(GameplayEffectsTree, SGameplayEffectsTree)
			.TreeItemsSource(&GameplayEffectsList)
			.OnGenerateRow_Lambda([](TSharedPtr<FGASGameplayEffectNodeBase> Item, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return
					SNew(SGASGameplayEffectTreeItem, OwnerTable)
					.WidgetInfoToVisualize(Item);
			})
			.OnGetChildren_Lambda([](TSharedPtr<FGASGameplayEffectNodeBase> Item, TArray<TSharedPtr<FGASGameplayEffectNodeBase>>& OutChildren)
			{
				OutChildren = Item->GetChildNodes();
			})
			.HighlightParentNodesForSelection(true)
			.HeaderRow
			(
				SNew(SHeaderRow)
				.CanSelectGeneratedColumn(true)

				+ SHeaderRow::Column(GameplayEffectNameColumn)
				.DefaultLabel(LOCTEXT("GameplayEffectNameColumn", "Name"))
				.DefaultTooltip(LOCTEXT("GameplayEffectNameColumnToolTip", "Gameplay Effect Name / Bonus Attribute"))
				.FillWidth(.2f)

				+ SHeaderRow::Column(GameplayEffectDurationColumn)
				.DefaultLabel(LOCTEXT("GameplayEffectDurationColumn", "Duration"))
				.FillWidth(.4f)

				+ SHeaderRow::Column(GameplayEffectStackColumn)
				.DefaultLabel(LOCTEXT("GameplayEffectStackColumn", "Stack"))
				.FillWidth(.1f)

				+ SHeaderRow::Column(GameplayEffectLevelColumn)
				.DefaultLabel(LOCTEXT("GameplayEffectLevelColumn", "Level"))
				.FillWidth(.1f)

				+ SHeaderRow::Column(GameplayEffectGrantedTagsColumn)
				.DefaultLabel(LOCTEXT("GameplayEffectGrantedTagsColumn", "Granted Tags"))
				.FillWidth(.2f)
			)
		]
	];
}

void SGASGameplayEffectsTab::Refresh(UAbilitySystemComponent* Component, const FName WorldContextHandle)
{
	GameplayEffectsList.Reset();

	TSet<FActiveGameplayEffectHandle> UnusedAbilities;
	MappedGameplayEffects.GetKeys(UnusedAbilities);

	if (Component)
	{
		for (auto It = Component->GetActiveGameplayEffects().CreateConstIterator(); It; ++It)
		{
			const FActiveGameplayEffect& ActiveGameplayEffect = *It;

			UnusedAbilities.Remove(ActiveGameplayEffect.Handle);
			if (const TSharedPtr<FGASGameplayEffectNodeBase>& AbilityNode = MappedGameplayEffects.FindRef(ActiveGameplayEffect.Handle))
			{
				AbilityNode->Update();
				continue;
			}

			TSharedRef<FGASGameplayEffectNode> NewItem = MakeShared<FGASGameplayEffectNode>(WorldContextHandle, Component, ActiveGameplayEffect.Handle);
			NewItem->Update();

			MappedGameplayEffects.Add(ActiveGameplayEffect.Handle, NewItem);
		}
	}

	for (const FActiveGameplayEffectHandle& UnusedAbility : UnusedAbilities)
	{
		MappedGameplayEffects.Remove(UnusedAbility);
	}

	MappedGameplayEffects.GenerateValueArray(GameplayEffectsList);

	GameplayEffectsTree->RequestTreeRefresh();
}

#undef LOCTEXT_NAMESPACE