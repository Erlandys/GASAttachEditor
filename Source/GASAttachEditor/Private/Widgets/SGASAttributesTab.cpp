// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAttributesTab.h"

#include "SGASAttributeItem.h"
#include "GASAttachEditorSettings.h"

#include "AbilitySystemComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

const TCHAR* SGASAttributesTab::HiddenCollectionsKey = TEXT("HiddenAttributeCollections");
const TCHAR* SGASAttributesTab::HiddenColumnsKey = TEXT("Attributes.HiddenColumns");
const TCHAR* SGASAttributesTab::HideZeroKey = TEXT("Attributes.HideZero");
const TCHAR* SGASAttributesTab::OnlyModifiedKey = TEXT("Attributes.OnlyModified");
const TCHAR* SGASAttributesTab::NameSortKey = TEXT("Attributes.NameSort");
const TCHAR* SGASAttributesTab::ValueSortKey = TEXT("Attributes.ValueSort");
const TCHAR* SGASAttributesTab::BaseValueSortKey = TEXT("Attributes.BaseValueSort");

const FName SGASAttributesTab::AttributeNameColumn = "Attribute_Name";
const FName SGASAttributesTab::AttributeValueColumn = "Attribute_Value";
const FName SGASAttributesTab::AttributeBaseValueColumn = "Attribute_BaseValue";

void SGASAttributesTab::Construct(const FArguments& InArgs)
{
	SearchFilter = MakeShared<FGASAttributeTextFilter>(FGASAttributeTextFilter::FItemToStringArray::CreateSP(this, &SGASAttributesTab::PopulateSearchStrings));

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
				CreateHideZeroCheckBox()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CreateOnlyModifiedCheckBox()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			[
				CreateCollectionsComboButton()
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
				SAssignNew(AttributesTree, SAttributesTree)
				.TreeItemsSource(&FilteredAttributesList)
				.OnGenerateRow_Lambda([this](TSharedPtr<FGASAttributeNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
				{
					return
						SNew(SGASAttributeItem, OwnerTable)
						.WidgetInfoToVisualize(Item)
						.HighlightText(this, &SGASAttributesTab::GetHighlightText);
				})
				.OnGetChildren_Lambda([this](TSharedPtr<FGASAttributeNode> Item, TArray<TSharedPtr<FGASAttributeNode>>& OutChildren)
				{
					const bool bCollectionMatchesText = MatchesText(*Item);
					for (const TSharedPtr<FGASAttributeNode>& ChildNode : Item->GetChildNodes())
					{
						if (ChildNode &&
							IsAttributeVisible(*ChildNode, bCollectionMatchesText))
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
					.OnHiddenColumnsListChanged(FSimpleDelegate::CreateSP(this, &SGASAttributesTab::SaveHiddenColumns))

					+ SHeaderRow::Column(AttributeNameColumn)
					.SortMode_Lambda([this]
					{
						return NameSortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						NameSortMode = InSortMode;
						ValueSortMode = EColumnSortMode::None;
						BaseValueSortMode = EColumnSortMode::None;
						SaveSettings();
						SortAttributes();
					})
					.DefaultLabel(LOCTEXT("AttributeNameColumn", "Name"))
					.DefaultTooltip(LOCTEXT("AttributeNameColumnToolTip", "Attribute set and the attributes it declares"))
					.FillWidth(.6f)
					.ShouldGenerateWidget(true)

					+ SHeaderRow::Column(AttributeValueColumn)
					.SortMode_Lambda([this]
					{
						return ValueSortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						NameSortMode = EColumnSortMode::None;
						ValueSortMode = InSortMode;
						BaseValueSortMode = EColumnSortMode::None;
						SaveSettings();
						SortAttributes();
					})
					.DefaultLabel(LOCTEXT("AttributeValueColumn", "Value"))
					.FillWidth(.2f)

					+ SHeaderRow::Column(AttributeBaseValueColumn)
					.SortMode_Lambda([this]
					{
						return BaseValueSortMode;
					})
					.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
					{
						NameSortMode = EColumnSortMode::None;
						ValueSortMode = EColumnSortMode::None;
						BaseValueSortMode = InSortMode;
						SaveSettings();
						SortAttributes();
					})
					.DefaultLabel(LOCTEXT("AttributeBaseValueColumn", "Base Value"))
					.FillWidth(.2f)
				)
			]
		]
	];
}

void SGASAttributesTab::Refresh(UAbilitySystemComponent* Component)
{
	AttributesList.Reset();

	TSet<FName> UnusedAttributes;
	MappedAttributes.GetKeys(UnusedAttributes);

	TSet<FName> UnusedCollections;
	MappedCollections.GetKeys(UnusedCollections);

	for (const TPair<FName, TSharedPtr<FGASAttributeNode>>& It : MappedCollections)
	{
		It.Value->ResetChildNodes();
	}

	if (Component)
	{
		for (const UAttributeSet* Set : Component->GetSpawnedAttributes())
		{
			if (!Set)
			{
				continue;
			}

			const FName CollectionKey = Set->GetClass()->GetFName();
			const FText CollectionName = FText::FromString(FName::NameToDisplayString(Set->GetClass()->GetName(), false));

			KnownCollections.Add(CollectionKey, CollectionName);

			TSharedPtr<FGASAttributeNode> CollectionNode = MappedCollections.FindRef(CollectionKey);
			if (!CollectionNode)
			{
				CollectionNode = MakeShared<FGASAttributeNode>(CollectionKey, CollectionName);
				MappedCollections.Add(CollectionKey, CollectionNode);
				AttributesTree->SetItemExpansion(CollectionNode, true);
			}
			UnusedCollections.Remove(CollectionKey);

			for (FStructProperty* Property : TFieldRange<FStructProperty>(Set->GetClass()))
			{
				if (!ensure(Property) ||
					!Property->Struct->IsChildOf(FGameplayAttributeData::StaticStruct()))
				{
					continue;
				}
				FName Key = *(Set->GetName() + Property->GetName());

				UnusedAttributes.Remove(Key);

				TSharedPtr<FGASAttributeNode> AttributeNode = MappedAttributes.FindRef(Key);
				if (!AttributeNode)
				{
					FGameplayAttribute Attribute(Property);

					AttributeNode = MakeShared<FGASAttributeNode>(Component, Attribute);
					MappedAttributes.Add(Key, AttributeNode);
				}

				AttributeNode->Update(Component);
				CollectionNode->AddChildNode(AttributeNode);
			}
		}
	}

	for (const FName UnusedAttribute : UnusedAttributes)
	{
		MappedAttributes.Remove(UnusedAttribute);
	}

	for (const FName UnusedCollection : UnusedCollections)
	{
		MappedCollections.Remove(UnusedCollection);
	}

	MappedCollections.GenerateValueArray(AttributesList);

	SortAttributes();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASAttributesTab::CreateSearchBox()
{
	return
		SAssignNew(SearchBox, SSearchBox)
		.HintText(LOCTEXT("AttributeSearchHint", "Search attributes and attribute sets"))
		.DelayChangeNotificationsWhileTyping(true)
		.OnTextChanged_Lambda([this](const FText& NewText)
		{
			SearchFilter->SetRawFilterText(NewText);
			SearchBox->SetError(SearchFilter->GetFilterErrorText());
			ApplyFilter();
		});
}

TSharedRef<SWidget> SGASAttributesTab::CreateCollectionsComboButton()
{
	return
		SNew(SComboButton)
		.ContentPadding(2.f)
		.VAlign(VAlign_Center)
		.ToolTipText(LOCTEXT("CollectionsToolTip", "Choose which attribute sets are shown. Persists across editor restarts."))
		.OnGetMenuContent(this, &SGASAttributesTab::BuildCollectionsMenu)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([this]
			{
				if (HiddenCollections.Num() == 0)
				{
					return LOCTEXT("CollectionsAll", "Collections: All");
				}

				return FText::Format(LOCTEXT("CollectionsHiddenFormat", "Collections: {0} hidden"), HiddenCollections.Num());
			})
		];
}

TSharedRef<SWidget> SGASAttributesTab::BuildCollectionsMenu()
{
	FMenuBuilder MenuBuilder(false, nullptr);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("CollectionsSection", "Attribute Sets"));

	TArray<TPair<FName, FText>> SortedCollections;
	for (const TPair<FName, FText>& It : KnownCollections)
	{
		SortedCollections.Add(It);
	}
	SortedCollections.Sort([](const TPair<FName, FText>& A, const TPair<FName, FText>& B)
	{
		return A.Value.ToString() < B.Value.ToString();
	});

	for (const FName& HiddenCollection : HiddenCollections)
	{
		if (!KnownCollections.Contains(HiddenCollection))
		{
			SortedCollections.Add({HiddenCollection, FText::FromName(HiddenCollection)});
		}
	}

	for (const TPair<FName, FText>& It : SortedCollections)
	{
		const FName CollectionKey = It.Key;

		MenuBuilder.AddMenuEntry(
			It.Value,
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SGASAttributesTab::ToggleCollectionHidden, CollectionKey),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SGASAttributesTab::IsCollectionShown, CollectionKey)),
			NAME_None,
			EUserInterfaceActionType::ToggleButton);
	}

	MenuBuilder.EndSection();

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CollectionsShowAll", "Show All"),
		LOCTEXT("CollectionsShowAllToolTip", "Stop hiding every attribute set"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SGASAttributesTab::ShowAllCollections),
			FCanExecuteAction::CreateLambda([this]
			{
				return HiddenCollections.Num() > 0;
			})));

	return MenuBuilder.MakeWidget();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool SGASAttributesTab::IsCollectionHidden(const FName CollectionKey) const
{
	return HiddenCollections.Contains(CollectionKey);
}

bool SGASAttributesTab::IsCollectionShown(const FName CollectionKey) const
{
	return !IsCollectionHidden(CollectionKey);
}

void SGASAttributesTab::ToggleCollectionHidden(const FName CollectionKey)
{
	if (HiddenCollections.Contains(CollectionKey))
	{
		HiddenCollections.Remove(CollectionKey);
	}
	else
	{
		HiddenCollections.Add(CollectionKey);
	}

	SaveSettings();

	ApplyFilter();
}

void SGASAttributesTab::ShowAllCollections()
{
	HiddenCollections.Reset();

	SaveSettings();

	ApplyFilter();
}

void SGASAttributesTab::LoadSettings()
{
	TSet<FName> HiddenColumnSet;
	FGASAttachEditorSettings::LoadNameSet(HiddenColumnsKey, HiddenColumnSet);
	HiddenColumns = HiddenColumnSet.Array();

	FGASAttachEditorSettings::LoadNameSet(HiddenCollectionsKey, HiddenCollections);

	bHideZero = FGASAttachEditorSettings::LoadBool(HideZeroKey, false);
	bOnlyModified = FGASAttachEditorSettings::LoadBool(OnlyModifiedKey, false);

	NameSortMode = FGASAttachEditorSettings::LoadSortMode(NameSortKey);
	ValueSortMode = FGASAttachEditorSettings::LoadSortMode(ValueSortKey);
	BaseValueSortMode = FGASAttachEditorSettings::LoadSortMode(BaseValueSortKey);
}

void SGASAttributesTab::SaveSettings() const
{
	FGASAttachEditorSettings::SaveNameSet(HiddenCollectionsKey, HiddenCollections);

	FGASAttachEditorSettings::SaveBool(HideZeroKey, bHideZero);
	FGASAttachEditorSettings::SaveBool(OnlyModifiedKey, bOnlyModified);

	FGASAttachEditorSettings::SaveSortMode(NameSortKey, NameSortMode);
	FGASAttachEditorSettings::SaveSortMode(ValueSortKey, ValueSortMode);
	FGASAttachEditorSettings::SaveSortMode(BaseValueSortKey, BaseValueSortMode);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SCheckBox> SGASAttributesTab::CreateHideZeroCheckBox()
{
	return
		SNew(SCheckBox)
		.Padding(FMargin(4.f, 0.f))
		.ToolTipText(LOCTEXT("HideZeroToolTip", "Hide attributes whose value and base value are both zero"))
		.IsChecked_Lambda([this]
		{
			return bHideZero ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewValue)
		{
			bHideZero = NewValue == ECheckBoxState::Checked;
			SaveSettings();
			ApplyFilter();
		})
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("HideZero", "Hide Zero"))
			]
		];
}

TSharedRef<SCheckBox> SGASAttributesTab::CreateOnlyModifiedCheckBox()
{
	return
		SNew(SCheckBox)
		.Padding(FMargin(4.f, 0.f))
		.ToolTipText(LOCTEXT("OnlyModifiedToolTip", "Only show attributes whose current value differs from their base value"))
		.IsChecked_Lambda([this]
		{
			return bOnlyModified ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewValue)
		{
			bOnlyModified = NewValue == ECheckBoxState::Checked;
			SaveSettings();
			ApplyFilter();
		})
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OnlyModified", "Only Modified"))
			]
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASAttributesTab::SortAttributes()
{
	const auto SortNodes = [](TArray<TSharedPtr<FGASAttributeNode>>& Nodes, const EColumnSortMode::Type SortMode, auto&& Projection)
	{
		if (SortMode == EColumnSortMode::None)
		{
			return;
		}

		const bool bAscending = SortMode == EColumnSortMode::Ascending;
		Nodes.Sort([&Projection, bAscending](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
		{
			return bAscending
				? Projection(A) < Projection(B)
				: Projection(B) < Projection(A);
		});
	};

	SortNodes(AttributesList, NameSortMode, [](const TSharedPtr<FGASAttributeNode>& Node) { return Node->GetCollectionName().ToString(); });

	for (const TSharedPtr<FGASAttributeNode>& CollectionNode : AttributesList)
	{
		if (!CollectionNode)
		{
			continue;
		}

		TArray<TSharedPtr<FGASAttributeNode>>& ChildNodes = CollectionNode->GetMutableChildNodes();

		SortNodes(ChildNodes, NameSortMode, [](const TSharedPtr<FGASAttributeNode>& Node) { return Node->GetName().ToString(); });
		SortNodes(ChildNodes, ValueSortMode, [](const TSharedPtr<FGASAttributeNode>& Node) { return Node->GetValue(); });
		SortNodes(ChildNodes, BaseValueSortMode, [](const TSharedPtr<FGASAttributeNode>& Node) { return Node->GetBaseValue(); });
	}

	ApplyFilter();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASAttributesTab::PopulateSearchStrings(const FGASAttributeNode& Node, TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Add(Node.GetCollectionName().ToString());

	if (Node.IsCollection())
	{
		OutSearchStrings.Add(Node.GetCollectionKey().ToString());
		return;
	}

	OutSearchStrings.Add(Node.GetName().ToString());
	OutSearchStrings.Add(Node.GetRawName());
}

FText SGASAttributesTab::GetHighlightText() const
{
	return SearchFilter->GetRawFilterText();
}

bool SGASAttributesTab::IsFilterActive() const
{
	return
		bHideZero ||
		bOnlyModified ||
		!SearchFilter->GetRawFilterText().IsEmpty();
}

bool SGASAttributesTab::MatchesText(const FGASAttributeNode& Node) const
{
	return SearchFilter->PassesFilter(Node);
}

bool SGASAttributesTab::PassesValueFilters(const FGASAttributeNode& Node) const
{
	if (bHideZero &&
		FMath::IsNearlyZero(Node.GetValue()) &&
		FMath::IsNearlyZero(Node.GetBaseValue()))
	{
		return false;
	}

	if (bOnlyModified &&
		FMath::IsNearlyEqual(Node.GetValue(), Node.GetBaseValue()))
	{
		return false;
	}

	return true;
}

bool SGASAttributesTab::IsAttributeVisible(const FGASAttributeNode& Node, const bool bCollectionMatchesText) const
{
	return
		PassesValueFilters(Node) &&
		(bCollectionMatchesText || MatchesText(Node));
}

bool SGASAttributesTab::HasVisibleAttributes(const FGASAttributeNode& CollectionNode, const bool bCollectionMatchesText) const
{
	for (const TSharedPtr<FGASAttributeNode>& ChildNode : CollectionNode.GetChildNodes())
	{
		if (ChildNode &&
			IsAttributeVisible(*ChildNode, bCollectionMatchesText))
		{
			return true;
		}
	}

	return false;
}

void SGASAttributesTab::ApplyFilter()
{
	FilteredAttributesList.Reset();

	const bool bFilterActive = IsFilterActive();
	for (const TSharedPtr<FGASAttributeNode>& CollectionNode : AttributesList)
	{
		if (!CollectionNode)
		{
			continue;
		}

		if (IsCollectionHidden(CollectionNode->GetCollectionKey()))
		{
			continue;
		}

		const bool bCollectionMatchesText = MatchesText(*CollectionNode);
		if (!HasVisibleAttributes(*CollectionNode, bCollectionMatchesText))
		{
			continue;
		}

		FilteredAttributesList.Add(CollectionNode);

		if (bFilterActive &&
			!bCollectionMatchesText)
		{
			AttributesTree->SetItemExpansion(CollectionNode, true);
		}
	}

	AttributesTree->RequestTreeRefresh();
}

void SGASAttributesTab::SaveHiddenColumns()
{
	if (!HeaderRow)
	{
		return;
	}

	HiddenColumns = HeaderRow->GetHiddenColumnIds();

	FGASAttachEditorSettings::SaveNameSet(HiddenColumnsKey, TSet<FName>(HiddenColumns));
}

#undef LOCTEXT_NAMESPACE