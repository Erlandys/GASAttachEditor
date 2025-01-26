#include "SGASAttributeItem.h"

#include "AbilitySystemComponent.h"
#include "Widgets/SGASAttributesTab.h"

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

FGASAttributeNode::FGASAttributeNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASComponent, const FGameplayAttribute& Attribute)
	: WeakComponent(ASComponent)
	, Attribute(Attribute)
{
}

void FGASAttributeNode::Update(UAbilitySystemComponent* NewComponent)
{
	WeakComponent = NewComponent;
	CollectionName = GatherCollectionName();
	Name = GatherName();
	ValueText = GatherValue(Value);
	BaseValueText = GatherBaseValue(BaseValue);
}

FText FGASAttributeNode::GatherCollectionName() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return LOCTEXT("None", "None");
	}

	return FText::FromString(Attribute.GetAttributeSetClass()->GetName());
}

FText FGASAttributeNode::GatherName() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return LOCTEXT("None", "None");
	}

	return FText::FromString(Attribute.GetName());
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
	SetPadding(0.f);

	check(WidgetInfo.IsValid());

	SMultiColumnTableRow<TSharedPtr<FGASAttributeNode>>::Construct(SMultiColumnTableRow<TSharedPtr<FGASAttributeNode>>::FArguments().Padding(0.f), InOwnerTableView);
}

TSharedRef<SWidget> SGASAttributeItem::GenerateWidgetForColumn(const FName& ColumnName)
{
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

	if (SGASAttributesTab::AttributeCollectionColumn == ColumnName)
	{
		Result->SetHAlign(HAlign_Left);
		TextField->SetJustification(ETextJustify::Left);
		TextField->SetText(MakeAttributeSP(WidgetInfo.Get(), &FGASAttributeNode::GetCollectionName));
	}
	else if (SGASAttributesTab::AttributeNameColumn == ColumnName)
	{
		Result->SetHAlign(HAlign_Left);
		TextField->SetJustification(ETextJustify::Left);
		TextField->SetText(MakeAttributeSP(WidgetInfo.Get(), &FGASAttributeNode::GetName));
	}
	else if (SGASAttributesTab::AttributeValueColumn == ColumnName)
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