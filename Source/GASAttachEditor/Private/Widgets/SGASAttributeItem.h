#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"

class UAbilitySystemComponent;

class FGASAttributeNode : public TSharedFromThis<FGASAttributeNode>
{
public:
	explicit FGASAttributeNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASComponent,const FGameplayAttribute& Attribute);

	void Update();

public:
	FORCEINLINE FText GetCollectionName() const { return CollectionName; }
	FORCEINLINE FText GetName() const { return Name; }
	FORCEINLINE float GetValue() const { return Value; }
	FORCEINLINE FText GetValueText() const { return ValueText; }
	FORCEINLINE float GetBaseValue() const { return BaseValue; }
	FORCEINLINE FText GetBaseValueText() const { return BaseValueText; }

private:
	FText GatherCollectionName() const;
	FText GatherName() const;
	FText GatherValue(float& OutValue) const;
	FText GatherBaseValue(float& OutValue) const;

private:
	FText CollectionName;
	FText Name;
	float Value = 0.f;
	FText ValueText;
	float BaseValue = 0.f;
	FText BaseValueText;

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
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

protected:
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TSharedPtr<FGASAttributeNode> WidgetInfo;
};