// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASGameplayEffectsTab.h"

#include "SGASGameplayEffectItem.h"
#include "GASAttachEditorSettings.h"

#include "AbilitySystemComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

const TCHAR* SGASGameplayEffectsTab::VisibleStatesKey = TEXT("GameplayEffects.VisibleStates");
const TCHAR* SGASGameplayEffectsTab::SortModeKey = TEXT("GameplayEffects.SortMode");
const TCHAR* SGASGameplayEffectsTab::HiddenColumnsKey = TEXT("GameplayEffects.HiddenColumns");

const FName SGASGameplayEffectsTab::GameplayEffectNameColumn = "GameplayEffect_Name";
const FName SGASGameplayEffectsTab::GameplayEffectStateColumn = "GameplayEffect_State";
const FName SGASGameplayEffectsTab::GameplayEffectDurationColumn = "GameplayEffect_Duration";
const FName SGASGameplayEffectsTab::GameplayEffectStackColumn = "GameplayEffect_Stack";
const FName SGASGameplayEffectsTab::GameplayEffectLevelColumn = "GameplayEffect_Level";
const FName SGASGameplayEffectsTab::GameplayEffectPredictionColumn = "GameplayEffect_Prediction";
const FName SGASGameplayEffectsTab::GameplayEffectGrantedTagsColumn = "GameplayEffect_GrantedTags";

void SGASGameplayEffectsTab::Construct(const FArguments& InArgs)
{
	SearchFilter = MakeShared<FGASGameplayEffectTextFilter>(FGASGameplayEffectTextFilter::FItemToStringArray::CreateSP(this, &SGASGameplayEffectsTab::PopulateSearchStrings));

	LoadSettings();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateStateSettingsCheckBox(EGameplayEffectStateType::Active)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateStateSettingsCheckBox(EGameplayEffectStateType::Inhibited)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateStateSettingsCheckBox(EGameplayEffectStateType::Infinite)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.f)
		[
			CreateSearchBox()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0.f)
			[
				SAssignNew(GameplayEffectsTree, SGameplayEffectsTree)
				.TreeItemsSource(&FilteredGameplayEffectsList)
				.OnGenerateRow_Lambda([this](TSharedPtr<FGASGameplayEffectNodeBase> Item, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return
						SNew(SGASGameplayEffectTreeItem, OwnerTable)
						.WidgetInfoToVisualize(Item)
						.HighlightText(this, &SGASGameplayEffectsTab::GetHighlightText);
				})
				.OnGetChildren_Lambda([this](TSharedPtr<FGASGameplayEffectNodeBase> Item, TArray<TSharedPtr<FGASGameplayEffectNodeBase>>& OutChildren)
				{
					// A directly matching effect shows all of its modifiers; otherwise only the matching ones
					const bool bParentMatches = MatchesText(*Item);
					for (const TSharedPtr<FGASGameplayEffectNodeBase>& ChildNode : Item->GetChildNodes())
					{
						if (bParentMatches ||
							PassesTextFilter(ChildNode))
						{
							OutChildren.Add(ChildNode);
						}
					}
				})
				.HighlightParentNodesForSelection(true)
				.HeaderRow
				(
					SAssignNew(HeaderRow, SHeaderRow)
					.CanSelectGeneratedColumn(true)
					.HiddenColumnsList(HiddenColumns)
					.OnHiddenColumnsListChanged(FSimpleDelegate::CreateSP(this, &SGASGameplayEffectsTab::SaveHiddenColumns))

					+ SHeaderRow::Column(GameplayEffectNameColumn)
					.SortMode_Lambda([this]
					{
						return SortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						SortMode = InSortMode;
						SaveSettings();
						SortGameplayEffects();
					})
					.DefaultLabel(LOCTEXT("GameplayEffectNameColumn", "Name"))
					.DefaultTooltip(LOCTEXT("GameplayEffectNameColumnToolTip", "Gameplay Effect Name / Bonus Attribute"))
					.FillWidth(.2f)

					+ SHeaderRow::Column(GameplayEffectStateColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectStateColumn", "State"))
					.FillWidth(.1f)

					+ SHeaderRow::Column(GameplayEffectDurationColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectDurationColumn", "Duration"))
					.FillWidth(.3f)

					+ SHeaderRow::Column(GameplayEffectStackColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectStackColumn", "Stack"))
					.FillWidth(.1f)

					+ SHeaderRow::Column(GameplayEffectLevelColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectLevelColumn", "Level"))
					.FillWidth(.1f)

					+ SHeaderRow::Column(GameplayEffectPredictionColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectPredictionColumn", "Prediction"))
					.DefaultTooltip(LOCTEXT("GameplayEffectPredictionColumnToolTip", "Client prediction state of this effect"))
					.FillWidth(.15f)

					+ SHeaderRow::Column(GameplayEffectGrantedTagsColumn)
					.DefaultLabel(LOCTEXT("GameplayEffectGrantedTagsColumn", "Granted Tags"))
					.FillWidth(.2f)
				)
			]
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

	SortGameplayEffects();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASGameplayEffectsTab::CreateSearchBox()
{
	return
		SAssignNew(SearchBox, SSearchBox)
		.HintText(LOCTEXT("GameplayEffectSearchHint", "Search effects, states and granted tags"))
		.DelayChangeNotificationsWhileTyping(true)
		.OnTextChanged_Lambda([this](const FText& NewText)
		{
			SearchFilter->SetRawFilterText(NewText);
			SearchBox->SetError(SearchFilter->GetFilterErrorText());
			ApplyFilter();
		});
}

TSharedRef<SCheckBox> SGASGameplayEffectsTab::CreateStateSettingsCheckBox(const EGameplayEffectStateType::Type StateType)
{
	FText StateTypeText;
	FText StateTypeToolTip;
	switch (StateType)
	{
	default: check(false);
	case EGameplayEffectStateType::Active:
		StateTypeText = LOCTEXT("GameplayEffectStateActive", "Active");
		StateTypeToolTip = LOCTEXT("GameplayEffectStateActiveToolTip", "Effects that are applying and have a finite duration");
		break;
	case EGameplayEffectStateType::Inhibited:
		StateTypeText = LOCTEXT("GameplayEffectStateInhibited", "Inhibited");
		StateTypeToolTip = LOCTEXT("GameplayEffectStateInhibitedToolTip", "Effects that are applied but currently suppressed by an ongoing requirement");
		break;
	case EGameplayEffectStateType::Infinite:
		StateTypeText = LOCTEXT("GameplayEffectStateInfinite", "Infinite");
		StateTypeToolTip = LOCTEXT("GameplayEffectStateInfiniteToolTip", "Effects that are applying and never expire on their own");
		break;
	}

	return
		SNew(SCheckBox)
		.Padding(FMargin(4.f, 0.f))
		.ToolTipText(StateTypeToolTip)
		.IsChecked_Lambda([this, StateType]
		{
			return (VisibleStateTypes & StateType) == StateType ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this, StateType](const ECheckBoxState NewValue)
		{
			switch (NewValue)
			{
			default: check(false);
			case ECheckBoxState::Unchecked: VisibleStateTypes ^= StateType; break;
			case ECheckBoxState::Checked: VisibleStateTypes |= StateType; break;
			case ECheckBoxState::Undetermined: break;
			}

			SaveSettings();
			ApplyFilter();
		})
		[
			SNew(SBox)
			.MinDesiredWidth(80.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(StateTypeText)
			]
		];
}

void SGASGameplayEffectsTab::SortGameplayEffects()
{
	if (SortMode == EColumnSortMode::Ascending)
	{
		GameplayEffectsList.Sort([](const TSharedPtr<FGASGameplayEffectNodeBase>& A, const TSharedPtr<FGASGameplayEffectNodeBase>& B)
		{
			return A->GetName().ToString() < B->GetName().ToString();
		});
	}
	else if (SortMode == EColumnSortMode::Descending)
	{
		GameplayEffectsList.Sort([](const TSharedPtr<FGASGameplayEffectNodeBase>& A, const TSharedPtr<FGASGameplayEffectNodeBase>& B)
		{
			return A->GetName().ToString() > B->GetName().ToString();
		});
	}

	ApplyFilter();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASGameplayEffectsTab::PopulateSearchStrings(const FGASGameplayEffectNodeBase& Node, TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Add(Node.GetName().ToString());
	OutSearchStrings.Add(Node.GetState().ToString());
	OutSearchStrings.Add(Node.GetGrantedTags().ToString());
}

FText SGASGameplayEffectsTab::GetHighlightText() const
{
	return SearchFilter->GetRawFilterText();
}

bool SGASGameplayEffectsTab::IsFilterActive() const
{
	return !SearchFilter->GetRawFilterText().IsEmpty();
}

bool SGASGameplayEffectsTab::MatchesText(const FGASGameplayEffectNodeBase& Node) const
{
	return SearchFilter->PassesFilter(Node);
}

bool SGASGameplayEffectsTab::PassesTextFilter(const TSharedPtr<FGASGameplayEffectNodeBase>& Node) const
{
	if (!Node)
	{
		return false;
	}

	if (MatchesText(*Node))
	{
		return true;
	}

	for (const TSharedPtr<FGASGameplayEffectNodeBase>& ChildNode : Node->GetChildNodes())
	{
		if (PassesTextFilter(ChildNode))
		{
			return true;
		}
	}

	return false;
}

void SGASGameplayEffectsTab::ApplyFilter()
{
	FilteredGameplayEffectsList.Reset();

	const bool bFilterActive = IsFilterActive();
	for (const TSharedPtr<FGASGameplayEffectNodeBase>& GameplayEffectNode : GameplayEffectsList)
	{
		if (!GameplayEffectNode)
		{
			continue;
		}

		if ((VisibleStateTypes & GameplayEffectNode->GetStateType()) == 0)
		{
			continue;
		}

		if (!PassesTextFilter(GameplayEffectNode))
		{
			continue;
		}

		FilteredGameplayEffectsList.Add(GameplayEffectNode);

		if (bFilterActive &&
			!MatchesText(*GameplayEffectNode))
		{
			GameplayEffectsTree->SetItemExpansion(GameplayEffectNode, true);
		}
	}

	GameplayEffectsTree->RequestTreeRefresh();
}

void SGASGameplayEffectsTab::LoadSettings()
{
	TSet<FName> HiddenColumnSet;
	FGASAttachEditorSettings::LoadNameSet(HiddenColumnsKey, HiddenColumnSet);
	HiddenColumns = HiddenColumnSet.Array();

	VisibleStateTypes = uint8(FGASAttachEditorSettings::LoadInt(VisibleStatesKey, EGameplayEffectStateType::MAX));
	SortMode = FGASAttachEditorSettings::LoadSortMode(SortModeKey);
}

void SGASGameplayEffectsTab::SaveSettings() const
{
	FGASAttachEditorSettings::SaveInt(VisibleStatesKey, VisibleStateTypes);
	FGASAttachEditorSettings::SaveSortMode(SortModeKey, SortMode);
}

void SGASGameplayEffectsTab::SaveHiddenColumns()
{
	if (!HeaderRow)
	{
		return;
	}

	HiddenColumns = HeaderRow->GetHiddenColumnIds();

	FGASAttachEditorSettings::SaveNameSet(HiddenColumnsKey, TSet<FName>(HiddenColumns));
}

#undef LOCTEXT_NAMESPACE