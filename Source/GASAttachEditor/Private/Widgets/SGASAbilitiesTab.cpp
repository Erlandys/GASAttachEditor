// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAbilitiesTab.h"

#include "SGASAbilityItem.h"
#include "AbilitySystemComponent.h"

#define LOCTEXT_NAMESPACE "SGASEditor"

const FName SGASAbilitiesTab::AbilityNameColumn = "Ability_Name";
const FName SGASAbilitiesTab::AbilityStateColumn = "Ability_State";
const FName SGASAbilitiesTab::AbilityActiveStateColumn = "Ability_ActiveState";
const FName SGASAbilitiesTab::AbilityTriggersColumn = "Ability_Triggers";

void SGASAbilitiesTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.f)
		.AutoHeight()
		.HAlign(HAlign_Left)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				CreateStateSettingsCheckBox(EAbilityStateType::Active)
			]
			+ SHorizontalBox::Slot()
			[
				CreateStateSettingsCheckBox(EAbilityStateType::Blocked)
			]
			+ SHorizontalBox::Slot()
			[
				CreateStateSettingsCheckBox(EAbilityStateType::Inactive)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0.f)
			[
				SAssignNew(AbilitiesTree, SAbilitiesTree)
				.TreeItemsSource(&AbilitiesList)
				.OnGenerateRow_Lambda([this](TSharedPtr<FGASAbilityNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return
						SNew(SGASAbilityItem, OwnerTable)
						.WidgetInfoToVisualize(Item)
						.ToolTipText(MakeAttributeSP(Item.Get(), &FGASAbilityNode::GetTriggersData));
				})
				.OnGetChildren_Lambda([this](TSharedPtr<FGASAbilityNode> Item, TArray<TSharedPtr<FGASAbilityNode>>& OutChildren)
				{
					OutChildren = Item->GetChildNodes();
				})
				.HighlightParentNodesForSelection(true)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.CanSelectGeneratedColumn(true)
					// .HiddenColumnsList(Hidden)
					.OnHiddenColumnsListChanged_Lambda([]{})
	
					+ SHeaderRow::Column(AbilityNameColumn)
					.SortMode_Lambda([this]
					{
						return SortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						SortMode = InSortMode;
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
				AbilityNode->Update(AbilitySpec, VisibleStateTypes);
				continue;
			}

			TSharedRef<FGASAbilityNode> NewItem = MakeShared<FGASAbilityNode>(Component, AbilitySpec);
			NewItem->Update(AbilitySpec, VisibleStateTypes);

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

			for (const TSharedPtr<FGASAbilityNode>& AbilityNode : AbilitiesList)
			{
				AbilityNode->UpdateVisibility(VisibleStateTypes);
			}
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
			return A->GetName().ToString() >= B->GetName().ToString();
		});
	}

	AbilitiesTree->RequestTreeRefresh();
}

#undef LOCTEXT_NAMESPACE