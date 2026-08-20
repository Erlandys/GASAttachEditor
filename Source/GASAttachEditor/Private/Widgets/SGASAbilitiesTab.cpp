// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAbilitiesTab.h"

#include "SGASAbilityItem.h"
#include "GASAttachEditorSettings.h"

#include "AbilitySystemComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

const TCHAR* SGASAbilitiesTab::VisibleStatesKey = TEXT("Abilities.VisibleStates");
const TCHAR* SGASAbilitiesTab::SortModeKey = TEXT("Abilities.SortMode");
const TCHAR* SGASAbilitiesTab::HiddenColumnsKey = TEXT("Abilities.HiddenColumns");

const FName SGASAbilitiesTab::AbilityNameColumn = "Ability_Name";
const FName SGASAbilitiesTab::AbilityStateColumn = "Ability_State";
const FName SGASAbilitiesTab::AbilityActiveStateColumn = "Ability_ActiveState";
const FName SGASAbilitiesTab::AbilityTriggersColumn = "Ability_Triggers";

void SGASAbilitiesTab::Construct(const FArguments& InArgs)
{
	SearchFilter = MakeShared<FGASAbilityTextFilter>(FGASAbilityTextFilter::FItemToStringArray::CreateSP(this, &SGASAbilitiesTab::PopulateSearchStrings));

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
				CreateStateSettingsCheckBox(EAbilityStateType::Active)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateStateSettingsCheckBox(EAbilityStateType::Blocked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateStateSettingsCheckBox(EAbilityStateType::Inactive)
			]
		]
		+ SVerticalBox::Slot()
		.Padding(2.f)
		.AutoHeight()
		[
			CreateSearchBox()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0.f)
			[
				SAssignNew(AbilitiesTree, SAbilitiesTree)
				.TreeItemsSource(&FilteredAbilitiesList)
				.OnGenerateRow_Lambda([this](TSharedPtr<FGASAbilityNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return
						SNew(SGASAbilityItem, OwnerTable)
						.WidgetInfoToVisualize(Item)
						.HighlightText(this, &SGASAbilitiesTab::GetHighlightText)
						.ToolTipText(MakeAttributeSP(Item.Get(), &FGASAbilityNode::GetTriggersData));
				})
				.OnGetChildren_Lambda([this](TSharedPtr<FGASAbilityNode> Item, TArray<TSharedPtr<FGASAbilityNode>>& OutChildren)
				{
					const bool bParentMatches = MatchesFilter(*Item);
					for (const TSharedPtr<FGASAbilityNode>& ChildNode : Item->GetChildNodes())
					{
						if (bParentMatches ||
							PassesFilter(ChildNode))
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
					.OnHiddenColumnsListChanged(FSimpleDelegate::CreateSP(this, &SGASAbilitiesTab::SaveHiddenColumns))

					+ SHeaderRow::Column(AbilityNameColumn)
					.SortMode_Lambda([this]
					{
						return SortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						SortMode = InSortMode;
						SaveSettings();
						SortAbilities();
					})
					.DefaultLabel(LOCTEXT("AbilityNameColumn", "Name"))
					.DefaultTooltip(LOCTEXT("AbilityNameColumnToolTip", "Ability or Ability Task Name"))
					.FillWidth(.5f)
					.ShouldGenerateWidget(true)

					+ SHeaderRow::Column(AbilityStateColumn)
					.DefaultLabel(LOCTEXT("AbilityStateColumn", "State"))
					.DefaultTooltip(LOCTEXT("AbilityStateColumnToolTip", "State of ability and reason if cannot be activated"))
					.FillWidth(.2f)

					+ SHeaderRow::Column(AbilityActiveStateColumn)
					.DefaultLabel(LOCTEXT("AbilityActiveStateColumn", "Is Active"))
					.FixedWidth(60.f)

					+ SHeaderRow::Column(AbilityTriggersColumn)
					.DefaultLabel(LOCTEXT("AbilityTriggersColumn", "Triggers"))
					.FillWidth(.3f)
				)
			]
		]
	];
}

void SGASAbilitiesTab::Refresh(UAbilitySystemComponent* Component)
{
	AbilitiesList.Reset();

	TSet<FGameplayAbilitySpecHandle> UnusedAbilities;
	MappedAbilities.GetKeys(UnusedAbilities);

	if (Component)
	{
		for (FGameplayAbilitySpec& AbilitySpec : Component->GetActivatableAbilities())
		{
			if (!AbilitySpec.Ability)
			{
				continue;
			}

			UnusedAbilities.Remove(AbilitySpec.Handle);
			if (const TSharedPtr<FGASAbilityNode>& AbilityNode = MappedAbilities.FindRef(AbilitySpec.Handle))
			{
				AbilityNode->Update();
				continue;
			}

			TSharedRef<FGASAbilityNode> NewItem = MakeShared<FGASAbilityNode>(Component, AbilitySpec.Handle);
			NewItem->Update();

			MappedAbilities.Add(AbilitySpec.Handle, NewItem);
		}
	}

	for (const FGameplayAbilitySpecHandle& UnusedAbility : UnusedAbilities)
	{
		MappedAbilities.Remove(UnusedAbility);
	}

	MappedAbilities.GenerateValueArray(AbilitiesList);

	SortAbilities();
}

TSharedRef<SWidget> SGASAbilitiesTab::CreateSearchBox()
{
	return
		SAssignNew(SearchBox, SSearchBox)
		.HintText(LOCTEXT("AbilitySearchHint", "Search abilities, states and triggers"))
		.DelayChangeNotificationsWhileTyping(true)
		.OnTextChanged_Lambda([this](const FText& NewText)
		{
			SearchFilter->SetRawFilterText(NewText);
			SearchBox->SetError(SearchFilter->GetFilterErrorText());
			ApplyFilter();
		});
}

TSharedRef<SCheckBox> SGASAbilitiesTab::CreateStateSettingsCheckBox(const EAbilityStateType::Type StateType)
{
	FText StateTypeText;
	switch (StateType)
	{
	default: check(false);
	case EAbilityStateType::Active: StateTypeText = LOCTEXT("AbilityActive", "Active"); break;
	case EAbilityStateType::Blocked: StateTypeText = LOCTEXT("AbilityBlocked", "Blocked"); break;
	case EAbilityStateType::Inactive: StateTypeText = LOCTEXT("AbilityInactive", "Inactive"); break;
	}

	return
		SNew(SCheckBox)
		.Padding(FMargin(4.f, 0.f))
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

void SGASAbilitiesTab::SortAbilities()
{
	if (SortMode == EColumnSortMode::Ascending)
	{
		AbilitiesList.Sort([](const TSharedPtr<FGASAbilityNode>& A, const TSharedPtr<FGASAbilityNode>& B)
		{
			return A->GetName().ToString() < B->GetName().ToString();
		});
	}
	else if (SortMode == EColumnSortMode::Descending)
	{
		AbilitiesList.Sort([](const TSharedPtr<FGASAbilityNode>& A, const TSharedPtr<FGASAbilityNode>& B)
		{
			return A->GetName().ToString() > B->GetName().ToString();
		});
	}

	ApplyFilter();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASAbilitiesTab::PopulateSearchStrings(const FGASAbilityNode& Node, TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Add(Node.GetName().ToString());
	OutSearchStrings.Add(Node.GetState().ToString());
	OutSearchStrings.Add(Node.GetTriggersData().ToString());
}

FText SGASAbilitiesTab::GetHighlightText() const
{
	return SearchFilter->GetRawFilterText();
}

bool SGASAbilitiesTab::IsFilterActive() const
{
	return !SearchFilter->GetRawFilterText().IsEmpty();
}

bool SGASAbilitiesTab::MatchesFilter(const FGASAbilityNode& Node) const
{
	if (Node.GetNodeType() == EGAAbilityNode::Ability &&
		(VisibleStateTypes & Node.GetStateType()) == 0)
	{
		return false;
	}

	return SearchFilter->PassesFilter(Node);
}

bool SGASAbilitiesTab::PassesFilter(const TSharedPtr<FGASAbilityNode>& Node) const
{
	if (!Node)
	{
		return false;
	}

	if (MatchesFilter(*Node))
	{
		return true;
	}

	for (const TSharedPtr<FGASAbilityNode>& ChildNode : Node->GetChildNodes())
	{
		if (PassesFilter(ChildNode))
		{
			return true;
		}
	}

	return false;
}

void SGASAbilitiesTab::ApplyFilter()
{
	FilteredAbilitiesList.Reset();

	const bool bFilterActive = IsFilterActive();
	for (const TSharedPtr<FGASAbilityNode>& AbilityNode : AbilitiesList)
	{
		if (!PassesFilter(AbilityNode))
		{
			continue;
		}

		FilteredAbilitiesList.Add(AbilityNode);

		if (bFilterActive &&
			!MatchesFilter(*AbilityNode))
		{
			AbilitiesTree->SetItemExpansion(AbilityNode, true);
		}
	}

	AbilitiesTree->RequestTreeRefresh();
}

void SGASAbilitiesTab::LoadSettings()
{
	TSet<FName> HiddenColumnSet;
	FGASAttachEditorSettings::LoadNameSet(HiddenColumnsKey, HiddenColumnSet);
	HiddenColumns = HiddenColumnSet.Array();

	VisibleStateTypes = static_cast<uint8>(FGASAttachEditorSettings::LoadInt(VisibleStatesKey, EAbilityStateType::MAX));
	SortMode = FGASAttachEditorSettings::LoadSortMode(SortModeKey);
}

void SGASAbilitiesTab::SaveSettings() const
{
	FGASAttachEditorSettings::SaveInt(VisibleStatesKey, VisibleStateTypes);
	FGASAttachEditorSettings::SaveSortMode(SortModeKey, SortMode);
}

void SGASAbilitiesTab::SaveHiddenColumns()
{
	if (!HeaderRow)
	{
		return;
	}

	HiddenColumns = HeaderRow->GetHiddenColumnIds();

	FGASAttachEditorSettings::SaveNameSet(HiddenColumnsKey, TSet<FName>(HiddenColumns));
}

#undef LOCTEXT_NAMESPACE