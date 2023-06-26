// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UAbilitySystemComponent;
class FGASAttributeNode;

using SAttributesTree = STreeView<TSharedPtr<FGASAttributeNode>>;

class GASATTACHEDITOR_API SGASAttributesTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASAttributesTab)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Refresh(UAbilitySystemComponent* Component);

private:
	void SortAttributes();

private:
	TSharedPtr<SAttributesTree> AttributesTree;

	EColumnSortMode::Type CollectionSortMode = EColumnSortMode::None;
	EColumnSortMode::Type NameSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ValueSortMode = EColumnSortMode::None;
	EColumnSortMode::Type BaseValueSortMode = EColumnSortMode::None;

private:
	TArray<TSharedPtr<FGASAttributeNode>> AttributesList;
	TMap<FName, TSharedPtr<FGASAttributeNode>> MappedAttributes;

public:
	static const FName AttributeCollectionColumn;
	static const FName AttributeNameColumn;
	static const FName AttributeValueColumn;
	static const FName AttributeBaseValueColumn;
};
