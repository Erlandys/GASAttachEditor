// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAttributeItem.h"
#include "AbilitySystemComponent.h"
#include "Widgets/SGASAttributesTab.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

FGASAttributeNode::FGASAttributeNode(const FName CollectionKey, const FText& CollectionName)
	: Type(EGASAttributeNode::Collection)
	, CollectionKey(CollectionKey)
	, CollectionName(CollectionName)
{
}

FGASAttributeNode::FGASAttributeNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASComponent, const FGameplayAttribute& Attribute)
	: Type(EGASAttributeNode::Attribute)
	, WeakComponent(ASComponent)
	, Attribute(Attribute)
{
	RawName = Attribute.GetName();
	Name = FText::FromString(FName::NameToDisplayString(RawName, false));

	if (const UClass* AttributeSetClass = Attribute.GetAttributeSetClass())
	{
		CollectionName = FText::FromString(FName::NameToDisplayString(AttributeSetClass->GetName(), false));
	}
	else
	{
		CollectionName = LOCTEXT("None", "None");
	}
}

void FGASAttributeNode::Update(UAbilitySystemComponent* NewComponent)
{
	if (Type == EGASAttributeNode::Collection)
	{
		return;
	}

	WeakComponent = NewComponent;
	ValueText = GatherValue(Value);
	BaseValueText = GatherBaseValue(BaseValue);
}

FText FGASAttributeNode::GatherValue(float& OutValue) const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		OutValue = 0.f;
		return FText::AsNumber(OutValue);
	}

	bool bFound = false;
	OutValue = Component->GetGameplayAttributeValue(Attribute, bFound);

	return FText::AsNumber(OutValue);
}

FText FGASAttributeNode::GatherBaseValue(float& OutValue) const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		OutValue = 0.f;
		return FText::AsNumber(OutValue);
	}

	OutValue = Component->GetNumericAttributeBase(Attribute);

	return FText::AsNumber(OutValue);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASAttributeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	WidgetInfo = InArgs._WidgetInfoToVisualize;
	HighlightText = InArgs._HighlightText;
	SetPadding(0.f);

	check(WidgetInfo.IsValid());

	SMultiColumnTableRow<TSharedPtr<FGASAttributeNode>>::Construct(SMultiColumnTableRow<TSharedPtr<FGASAttributeNode>>::FArguments().Padding(0.f), InOwnerTableView);
}

TSharedRef<SWidget> SGASAttributeItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	const bool bIsCollection = WidgetInfo->IsCollection();

	if (SGASAttributesTab::AttributeNameColumn == ColumnName)
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(16)
				.ShouldDrawWires(true)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(MakeAttributeSP(WidgetInfo.Get(), bIsCollection ? &FGASAttributeNode::GetCollectionName : &FGASAttributeNode::GetName))
				.HighlightText(HighlightText)
				.Justification(ETextJustify::Left)
			];
	}

	if (bIsCollection)
	{
		return SNullWidget::NullWidget;
	}

	TSharedPtr<STextBlock> TextField;

	TSharedRef<SBox> Result =
		SNew(SBox)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(2.0f, 0.0f)
		[
			SAssignNew(TextField, STextBlock)
			.Justification(ETextJustify::Center)
		];

	if (SGASAttributesTab::AttributeValueColumn == ColumnName)
	{
		TextField->SetText(MakeAttributeSP(WidgetInfo.Get(), &FGASAttributeNode::GetValueText));
	}
	else if (SGASAttributesTab::AttributeBaseValueColumn == ColumnName)
	{
		TextField->SetText(MakeAttributeSP(WidgetInfo.Get(), &FGASAttributeNode::GetBaseValueText));
	}
	else
	{
		ensure(false);
		return SNullWidget::NullWidget;
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE