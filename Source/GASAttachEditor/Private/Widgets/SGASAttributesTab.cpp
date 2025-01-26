// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAttributesTab.h"

#include "SGASAttributeItem.h"
#include "AbilitySystemComponent.h"

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

const FName SGASAttributesTab::AttributeCollectionColumn = "Attribute_Collection";
const FName SGASAttributesTab::AttributeNameColumn = "Attribute_Name";
const FName SGASAttributesTab::AttributeValueColumn = "Attribute_Value";
const FName SGASAttributesTab::AttributeBaseValueColumn = "Attribute_BaseValue";

void SGASAttributesTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(0.f)
		[
			SAssignNew(AttributesTree, SAttributesTree)
			.TreeItemsSource(&AttributesList)
			.OnGenerateRow_Lambda([this](TSharedPtr<FGASAttributeNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return
					SNew(SGASAttributeItem, OwnerTable)
					.WidgetInfoToVisualize(Item);
			})
			.OnGetChildren_Lambda([](TSharedPtr<FGASAttributeNode>, TArray<TSharedPtr<FGASAttributeNode>>&)
			{
			})
			.HighlightParentNodesForSelection(true)
			.HeaderRow
			(
				SNew(SHeaderRow)
				.CanSelectGeneratedColumn(true)

				+ SHeaderRow::Column(AttributeCollectionColumn)
				.SortMode_Lambda([this]
				{
					return CollectionSortMode;
				})
				.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
				{
					NameSortMode = EColumnSortMode::None;
					CollectionSortMode = InSortMode;
					ValueSortMode = EColumnSortMode::None;
					BaseValueSortMode = EColumnSortMode::None;
					SortAttributes();
				})
				.DefaultLabel(LOCTEXT("AttributeCollectionColumn", "Collection"))
				.FillWidth(.3f)

				+ SHeaderRow::Column(AttributeNameColumn)
				.SortMode_Lambda([this]
				{
					return NameSortMode;
				})
				.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
				{
					NameSortMode = InSortMode;
					CollectionSortMode = EColumnSortMode::None;
					ValueSortMode = EColumnSortMode::None;
					BaseValueSortMode = EColumnSortMode::None;
					SortAttributes();
				})
				.DefaultLabel(LOCTEXT("AttributeNameColumn", "Name"))
				.FillWidth(.3f)
				.ShouldGenerateWidget(true)

				+ SHeaderRow::Column(AttributeValueColumn)
				.SortMode_Lambda([this]
				{
					return ValueSortMode;
				})
				.OnSort_Lambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode)
				{
					NameSortMode = EColumnSortMode::None;
					CollectionSortMode = EColumnSortMode::None;
					ValueSortMode = InSortMode;
					BaseValueSortMode = EColumnSortMode::None;
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
					CollectionSortMode = EColumnSortMode::None;
					ValueSortMode = EColumnSortMode::None;
					BaseValueSortMode = InSortMode;
					SortAttributes();
				})
				.DefaultLabel(LOCTEXT("AttributeBaseValueColumn", "Base Value"))
				.FillWidth(.2f)
			)
		]
	];
}

void SGASAttributesTab::Refresh(UAbilitySystemComponent* Component)
{
	AttributesList.Reset();

	TSet<FName> UnusedAttributes;
	MappedAttributes.GetKeys(UnusedAttributes);

	if (Component)
	{
		for (const UAttributeSet* Set : Component->GetSpawnedAttributes())
		{
			if (!Set)
			{
				continue;
			}

			for (FStructProperty* Property : TFieldRange<FStructProperty>(Set->GetClass()))
			{
				if (!ensure(Property) ||
					!Property->Struct->IsChildOf(FGameplayAttributeData::StaticStruct()))
				{
					continue;
				}
				FName Key = *(Set->GetName() + Property->GetName());

				UnusedAttributes.Remove(Key);
				if (const TSharedPtr<FGASAttributeNode>& AttributeNode = MappedAttributes.FindRef(Key))
				{
					AttributeNode->Update(Component);
					continue;
				}

				FGameplayAttribute Attribute(Property);

				TSharedRef<FGASAttributeNode> NewItem = MakeShared<FGASAttributeNode>(Component, Attribute);
				NewItem->Update(Component);

				MappedAttributes.Add(Key, NewItem);
			}
		}
	}

	for (const FName UnusedAttribute : UnusedAttributes)
	{
		MappedAttributes.Remove(UnusedAttribute);
	}

	MappedAttributes.GenerateValueArray(AttributesList);

	SortAttributes();
}

void SGASAttributesTab::SortAttributes()
{
	if (CollectionSortMode != EColumnSortMode::None)
	{
		if (CollectionSortMode == EColumnSortMode::Ascending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetCollectionName().ToString() < B->GetCollectionName().ToString();
			});
		}
		else if (CollectionSortMode == EColumnSortMode::Descending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetCollectionName().ToString() >= B->GetCollectionName().ToString();
			});
		}
	}

	if (NameSortMode != EColumnSortMode::None)
	{
		if (NameSortMode == EColumnSortMode::Ascending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetName().ToString() < B->GetName().ToString();
			});
		}
		else if (NameSortMode == EColumnSortMode::Descending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetName().ToString() >= B->GetName().ToString();
			});
		}
	}

	if (ValueSortMode != EColumnSortMode::None)
	{
		if (ValueSortMode == EColumnSortMode::Ascending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetValue() < B->GetValue();
			});
		}
		else if (ValueSortMode == EColumnSortMode::Descending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetValue() >= B->GetValue();
			});
		}
	}

	if (BaseValueSortMode != EColumnSortMode::None)
	{
		if (BaseValueSortMode == EColumnSortMode::Ascending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetBaseValue() < B->GetBaseValue();
			});
		}
		else if (BaseValueSortMode == EColumnSortMode::Descending)
		{
			AttributesList.Sort([](const TSharedPtr<FGASAttributeNode>& A, const TSharedPtr<FGASAttributeNode>& B)
			{
				return A->GetBaseValue() >= B->GetBaseValue();
			});
		}
	}

	AttributesTree->RequestTreeRefresh();
}

#undef LOCTEXT_NAMESPACE