// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Widgets/Views/STableRow.h"

class UAbilitySystemComponent;

enum class EGASAttributeNode
{
	Collection,
	Attribute,
};

class FGASAttributeNode : public TSharedFromThis<FGASAttributeNode>
{
public:
	explicit FGASAttributeNode(FName CollectionKey, const FText& CollectionName);
	explicit FGASAttributeNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASComponent, const FGameplayAttribute& Attribute);

	void Update(UAbilitySystemComponent* NewComponent);

public:
	FORCEINLINE EGASAttributeNode GetNodeType() const { return Type; }
	FORCEINLINE bool IsCollection() const { return Type == EGASAttributeNode::Collection; }

	FORCEINLINE FName GetCollectionKey() const { return CollectionKey; }
	FORCEINLINE FText GetCollectionName() const { return CollectionName; }
	FORCEINLINE const FString& GetRawName() const { return RawName; }
	FORCEINLINE FText GetName() const { return Name; }
	FORCEINLINE float GetValue() const { return Value; }
	FORCEINLINE FText GetValueText() const { return ValueText; }
	FORCEINLINE float GetBaseValue() const { return BaseValue; }
	FORCEINLINE FText GetBaseValueText() const { return BaseValueText; }

	const TArray<TSharedPtr<FGASAttributeNode>>& GetChildNodes() const { return ChildNodes; }
	void ResetChildNodes() { ChildNodes.Reset(); }
	void AddChildNode(const TSharedPtr<FGASAttributeNode>& ChildNode) { ChildNodes.Add(ChildNode); }
	TArray<TSharedPtr<FGASAttributeNode>>& GetMutableChildNodes() { return ChildNodes; }

private:
	FText GatherValue(float& OutValue) const;
	FText GatherBaseValue(float& OutValue) const;

private:
	EGASAttributeNode Type = EGASAttributeNode::Attribute;

	FName CollectionKey;
	FText CollectionName;
	FText Name;
	FString RawName;
	float Value = 0.f;
	FText ValueText;
	float BaseValue = 0.f;
	FText BaseValueText;

	TArray<TSharedPtr<FGASAttributeNode>> ChildNodes;

private:
	TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	FGameplayAttribute Attribute;
};


class SGASAttributeItem : public SMultiColumnTableRow<TSharedPtr<FGASAttributeNode>>
{
public:
	SLATE_BEGIN_ARGS(SGASAttributeItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASAttributeNode>, WidgetInfoToVisualize)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

protected:
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FGASAttributeNode> WidgetInfo;
	TAttribute<FText> HighlightText;
};