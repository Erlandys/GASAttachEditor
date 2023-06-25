#include "SGASGameplayTagsItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

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

	FString Base = GatherName().ToString();

#if WITH_EDITOR
	FString Comment;
	FName TagSource;
	bool bIsTagExplicit = false;
	bool bIsRestrictedTag = false;
	bool bAllowNonRestrictedChildren = false;
	if (UGameplayTagsManager::Get().GetTagEditorData(*Tag.ToString(), Comment, TagSource, bIsTagExplicit, bIsRestrictedTag, bAllowNonRestrictedChildren))
	{
		if (bIsTagExplicit)
		{
			Base += " (" + TagSource.ToString() + ")";
		}
		else
		{
			Base += " Implicit";
		}

		if (!Comment.IsEmpty())
		{
			Base += "\n\n" + Comment;
		}
	}
#endif

	FString TagAbilities;
	for (const FGameplayAbilitySpec& AbilitySpec : Component->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() ||
			!AbilitySpec.Ability)
		{
			continue;
		}

		const FStructProperty* Property = FindFProperty<FStructProperty>(AbilitySpec.Ability->GetClass(), PropertyName);
		if (!Property ||
			Property->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}

		const FGameplayTagContainer* ActivationTags = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilitySpec.Ability);
		if (!ActivationTags)
		{
			continue;
		}

		if (ActivationTags->HasTag(Tag))
		{
			TagAbilities += Component->CleanupName(GetNameSafe(AbilitySpec.Ability)) + ", ";
		}
	}

	if (!TagAbilities.IsEmpty())
	{
		TagAbilities.RemoveFromEnd(", ");
		Base += "\n\n" + TagAbilities;
	}

	return FText::FromString(Base);
}

FString FGASTagNode::GatherTagName() const
{
	return Tag.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASTagView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (FSlateApplication::Get().IsDragDropping())
	{
		return;
	}

	STileView<TSharedPtr<FGASTagNode>>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

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