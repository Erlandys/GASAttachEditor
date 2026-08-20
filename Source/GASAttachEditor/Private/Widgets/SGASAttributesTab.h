// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/TextFilter.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/SHeaderRow.h"

class SCheckBox;
class SSearchBox;
class FGASAttributeNode;
class UAbilitySystemComponent;

using SAttributesTree = STreeView<TSharedPtr<FGASAttributeNode>>;
using FGASAttributeTextFilter = TTextFilter<const FGASAttributeNode&>;

class SGASAttributesTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASAttributesTab)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Refresh(UAbilitySystemComponent* Component);

private:
	TSharedRef<SWidget> CreateSearchBox();
	TSharedRef<SCheckBox> CreateHideZeroCheckBox();
	TSharedRef<SCheckBox> CreateOnlyModifiedCheckBox();
	TSharedRef<SWidget> CreateCollectionsComboButton();
	TSharedRef<SWidget> BuildCollectionsMenu();

	bool IsCollectionHidden(FName CollectionKey) const;
	bool IsCollectionShown(FName CollectionKey) const;
	void ToggleCollectionHidden(FName CollectionKey);
	void ShowAllCollections();

	void SaveHiddenColumns();

	void LoadSettings();
	void SaveSettings() const;

	static const TCHAR* HiddenCollectionsKey;
	static const TCHAR* HiddenColumnsKey;
	static const TCHAR* HideZeroKey;
	static const TCHAR* OnlyModifiedKey;
	static const TCHAR* NameSortKey;
	static const TCHAR* ValueSortKey;
	static const TCHAR* BaseValueSortKey;

	void SortAttributes();

private:
	void PopulateSearchStrings(const FGASAttributeNode& Node, TArray<FString>& OutSearchStrings) const;
	FText GetHighlightText() const;

	bool IsFilterActive() const;
	bool MatchesText(const FGASAttributeNode& Node) const;
	bool PassesValueFilters(const FGASAttributeNode& Node) const;
	bool IsAttributeVisible(const FGASAttributeNode& Node, bool bCollectionMatchesText) const;
	bool HasVisibleAttributes(const FGASAttributeNode& CollectionNode, bool bCollectionMatchesText) const;
	void ApplyFilter();

private:
	TSharedPtr<SAttributesTree> AttributesTree;
	TSharedPtr<SHeaderRow> HeaderRow;
	TArray<FName> HiddenColumns;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<FGASAttributeTextFilter> SearchFilter;

	EColumnSortMode::Type NameSortMode = EColumnSortMode::None;
	EColumnSortMode::Type ValueSortMode = EColumnSortMode::None;
	EColumnSortMode::Type BaseValueSortMode = EColumnSortMode::None;

	bool bHideZero = false;
	bool bOnlyModified = false;

	TSet<FName> HiddenCollections;
	TMap<FName, FText> KnownCollections;

private:
	TArray<TSharedPtr<FGASAttributeNode>> AttributesList;
	TArray<TSharedPtr<FGASAttributeNode>> FilteredAttributesList;
	TMap<FName, TSharedPtr<FGASAttributeNode>> MappedAttributes;
	TMap<FName, TSharedPtr<FGASAttributeNode>> MappedCollections;

public:
	static const FName AttributeNameColumn;
	static const FName AttributeValueColumn;
	static const FName AttributeBaseValueColumn;
};