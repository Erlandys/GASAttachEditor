// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASGameplayTagsItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

FGASTagNode::FGASTagNode(const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FGameplayTag& Tag, FName PropertyName)
	: WeakComponent(WeakComponent)
	, Tag(Tag)
	, PropertyName(PropertyName)
{
}

void FGASTagNode::Update()
{
	Name = GatherName();
	ToolTip = GatherToolTip();
	TagName = GatherTagName();
}

FText FGASTagNode::GatherName() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return LOCTEXT("None", "None");
	}

	return FText::Format(
		LOCTEXT("TagNameFormat", "{0} [{1}]"),
		FText::FromString(Tag.ToString()),
		FText::AsNumber(Component->GetGameplayTagCount(Tag))); // TODO: Probably will be incorrect for blocked tags?
}

FText FGASTagNode::GatherToolTip() const
{
	UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return LOCTEXT("None", "None");
	}

	TArray<FText> ToolTipSections;
	FText Header = GatherName();

#if WITH_EDITOR
	FString Comment;
	FName TagSource;
	bool bIsTagExplicit = false;
	bool bIsRestrictedTag = false;
	bool bAllowNonRestrictedChildren = false;
	if (UGameplayTagsManager::Get().GetTagEditorData(*Tag.ToString(), Comment, TagSource, bIsTagExplicit, bIsRestrictedTag, bAllowNonRestrictedChildren))
	{
		Header = bIsTagExplicit
			? FText::Format(LOCTEXT("TagSourceFormat", "{0} ({1})"), Header, FText::FromName(TagSource))
			: FText::Format(LOCTEXT("TagImplicitFormat", "{0} Implicit"), Header);

		if (!Comment.IsEmpty())
		{
			ToolTipSections.Add(FText::FromString(Comment));
		}
	}
#endif

	ToolTipSections.Insert(Header, 0);

	TArray<FText> TagAbilityNames;
	for (const FGameplayAbilitySpec& AbilitySpec : Component->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() ||
			!AbilitySpec.Ability)
		{
			continue;
		}

		UGameplayAbility* Ability = AbilitySpec.Ability;
		for (UGameplayAbility* InstancedAbility : AbilitySpec.GetAbilityInstances())
		{
			if (InstancedAbility)
			{
				Ability = InstancedAbility;
				break;
			}
		}

		const FStructProperty* Property = FindFProperty<FStructProperty>(Ability->GetClass(), PropertyName);
		if (!Property ||
			Property->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}

		const FGameplayTagContainer* ActivationTags = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Ability);
		if (!ActivationTags)
		{
			continue;
		}

		if (ActivationTags->HasTag(Tag))
		{
			TagAbilityNames.Add(FText::FromString(Component->CleanupName(GetNameSafe(Ability))));
		}
	}

	if (TagAbilityNames.Num() > 0)
	{
		ToolTipSections.Add(FText::Join(LOCTEXT("TagAbilitySeparator", ", "), TagAbilityNames));
	}

	return FText::Join(FText::FromString(TEXT("\n\n")), ToolTipSections);
}

FString FGASTagNode::GatherTagName() const
{
	return Tag.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASTagViewItem::Construct(const FArguments& InArgs)
{
	TagNode = InArgs._TagNode;

	FTextBlockStyle TextStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	TextStyle.ColorAndOpacity = FSlateColor(FLinearColor::White);

	ChildSlot
	[
		SNew(SButton)
#if WITH_EDITOR
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
#endif
		.ToolTipText(MakeAttributeSP(TagNode.Get(), &FGASTagNode::GetToolTip))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked(this, &SGASTagViewItem::HandleOnClicked)
		[
			SNew(STextBlock)
			.Text(MakeAttributeSP(TagNode.Get(), &FGASTagNode::GetName))
			.TextStyle(&TextStyle)
		]
	];
}

FReply SGASTagViewItem::HandleOnClicked() const
{
	if (!ensure(TagNode))
	{
		return FReply::Handled();
	}

	FPlatformApplicationMisc::ClipboardCopy(*TagNode->GetTagName());
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE