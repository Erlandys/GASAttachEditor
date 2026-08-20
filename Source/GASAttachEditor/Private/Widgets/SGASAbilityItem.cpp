// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASAbilityItem.h"

#include "GASAttachEditorAbilityAccessors.h"
#include "Styling/StyleColors.h"
#include "AbilitySystemComponent.h"
#include "Widgets/Input/SHyperlink.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "GASAttachEditor"

FGASAbilityNode::FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle)
	: Type(EGAAbilityNode::Ability)
	, AbilitySpecHandle(AbilitySpecHandle)
	, WeakComponent(ASC)
{
}

FGASAbilityNode::FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const TWeakObjectPtr<UGameplayTask>& InGameplayTask)
	: Type(EGAAbilityNode::Task)
	, AbilitySpecHandle(AbilitySpecHandle)
	, WeakComponent(ASC)
	, GameplayTask(InGameplayTask)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FGASAbilityNode::Update()
{
	Name = FetchName();
	State = FetchState(StateType);
	TriggersData = FetchTriggersData();
	ActiveState = IsActive() ? LOCTEXT("AbilityIsActiveYes", "Yes") : LOCTEXT("AbilityIsActiveNo", "No");
	FetchSourceAsset();
	FixupColor();

	FixupTasks();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FGameplayAbilitySpec* FGASAbilityNode::FindAbilitySpec() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return nullptr;
	}

	return Component->FindAbilitySpecFromHandle(AbilitySpecHandle);
}

UGameplayAbility* FGASAbilityNode::FindAbility() const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpec();
	if (!AbilitySpec)
	{
		return nullptr;
	}

	for (UGameplayAbility* Ability : AbilitySpec->GetAbilityInstances())
	{
		if (Ability)
		{
			return Ability;
		}
	}

	return AbilitySpec->Ability;
}

bool FGASAbilityNode::IsActive() const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpec();

	return
		AbilitySpec &&
		AbilitySpec->IsActive();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText FGASAbilityNode::FetchName() const
{
	UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!ensure(Component))
	{
		return {};
	}

	if (Type == EGAAbilityNode::Task)
	{
		return FText::FromString(GameplayTask.IsValid() ? GameplayTask->GetDebugString() : "");
	}
	if (Type == EGAAbilityNode::Ability)
	{
		return FText::FromString(Component->CleanupName(GetNameSafe(FindAbility())));
	}

	ensure(false);
	return {};
}

FText FGASAbilityNode::FetchState(EAbilityStateType::Type &OutStateType) const
{
	if (Type != EGAAbilityNode::Ability)
	{
		return {};
	}

	OutStateType = EAbilityStateType::Inactive;
	if (!WeakComponent.IsValid())
	{
		return {};
	}

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpec();
	if (!AbilitySpec)
	{
		return {};
	}

	if (AbilitySpec->IsActive())
	{
		OutStateType = EAbilityStateType::Active;

		return FText::Format(LOCTEXT("ActiveCountFormat", "Active Count: {0}"), AbilitySpec->ActiveCount);
	}

	if (WeakComponent->IsAbilityInputBlocked(AbilitySpec->InputID))
	{
		OutStateType = EAbilityStateType::Blocked;

		return LOCTEXT("InputBlocked", "Input Blocked");
	}

	const UGameplayAbility* Ability = FindAbility();
	if (!Ability)
	{
		return {};
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	if (WeakComponent->AreAbilityTagsBlocked(Ability->GetAssetTags()))
#else
	if (WeakComponent->AreAbilityTagsBlocked(Ability->AbilityTags))
#endif
	{
		OutStateType = EAbilityStateType::Blocked;

		return LOCTEXT("TagBlocked", "Blocked Tags");
	}

	FGameplayTagContainer FailureTags;
	if (!Ability->CanActivateAbility(AbilitySpecHandle, WeakComponent->AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags))
	{
		OutStateType = EAbilityStateType::Blocked;

		const float Cooldown = Ability->GetCooldownTimeRemaining(WeakComponent->AbilityActorInfo.Get());
		if (Cooldown > 0.f)
		{
			FNumberFormattingOptions NumberFormatOptions;
			NumberFormatOptions.MaximumFractionalDigits = 2;

			return FText::Format(
				LOCTEXT("CantActivateCooldownFormat", "Can't Activate, Cooldown Time: {0}s"),
				FText::AsNumber(Cooldown, &NumberFormatOptions));
		}

		return LOCTEXT("CantActivate", "Can't Activate");
	}

	return {};
}

FText FGASAbilityNode::FetchTriggersData() const
{
	if (!WeakComponent.IsValid())
	{
		return {};
	}

	UGameplayAbility* Ability = FindAbility();
	if (!Ability)
	{
		return {};
	}

	const TArray<FAbilityTriggerData>* ActivationTagsPtr = FGASAbilityAccessors::FindAbilityTriggers(Ability);
	if (!ensure(ActivationTagsPtr))
	{
		return {};
	}

	TArray<FText> TriggerTexts;
	for (const FAbilityTriggerData& TriggerData : *ActivationTagsPtr)
	{
		TriggerTexts.Add(FText::Format(
			LOCTEXT("AbilityTriggerFormat", "Tag: ({0}), Event: ({1})"),
			FText::FromName(TriggerData.TriggerTag.GetTagName()),
			UEnum::GetDisplayValueAsText(TriggerData.TriggerSource)));
	}

	return FText::Join(FText::FromString(TEXT("\n")), TriggerTexts);
}

void FGASAbilityNode::FetchSourceAsset()
{
	// Resolve once - after that it must survive the ability, the component and PIE itself
	if (SourceAsset.IsValid() ||
		Type != EGAAbilityNode::Ability)
	{
		return;
	}

	SourceAsset = FGASSourceAsset::FromObject(FindAbility());
}

void FGASAbilityNode::FixupColor()
{
	switch (StateType)
	{
	default: check(false);
	case EAbilityStateType::Active: Tint = FSlateColor::UseForeground().GetColor(FWidgetStyle()); break;
	case EAbilityStateType::Blocked: Tint = FStyleColors::Error.GetSpecifiedColor(); break;
	case EAbilityStateType::Inactive: Tint = FSlateColor::UseSubduedForeground().GetColor(FWidgetStyle()); break;
	}
}

void FGASAbilityNode::FixupTasks()
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpec();
	if (!WeakComponent.IsValid() ||
		!AbilitySpec ||
		!AbilitySpec->IsActive() ||
		Type != EGAAbilityNode::Ability)
	{
		MappedChildNodes = {};
		ChildNodes = {};
		return;
	}

	TSet<FObjectKey> InactiveTasks;
	MappedChildNodes.GetKeys(InactiveTasks);

	TArray<UGameplayAbility*> Instances = AbilitySpec->GetAbilityInstances();
	for (UGameplayAbility* Instance : Instances)
	{
		if (!ensure(Instance))
		{
			continue;
		}

		const TArray<TObjectPtr<UGameplayTask>>* ActiveTasksPtr = FGASAbilityAccessors::FindActiveTasks(Instance);
		if (!ensure(ActiveTasksPtr))
		{
			continue;
		}

		TArray<TObjectPtr<UGameplayTask>> ActiveTasks = *ActiveTasksPtr;
		for (UGameplayTask* Item : ActiveTasks)
		{
			if (!Item)
			{
				continue;
			}

			const FObjectKey Key(Item);
			InactiveTasks.Remove(Key);

			if (const TSharedPtr<FGASAbilityNode>& Task = MappedChildNodes.FindRef(Key))
			{
				Task->GameplayTask = Item;
				Task->Update();
				continue;
			}

			TSharedRef<FGASAbilityNode> NewTask = MakeShared<FGASAbilityNode>(WeakComponent, AbilitySpecHandle, Item);
			NewTask->Update();
			MappedChildNodes.Add(Key, NewTask);
		}
	}

	for (const FObjectKey& InactiveTask : InactiveTasks)
	{
		MappedChildNodes.Remove(InactiveTask);
	}

	MappedChildNodes.GenerateValueArray(ChildNodes);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const TArray<TSharedPtr<FGASAbilityNode>>& FGASAbilityNode::GetChildNodes() const
{
	return ChildNodes;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASAbilityItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	WidgetInfo = InArgs._WidgetInfoToVisualize;
	HighlightText = InArgs._HighlightText;
	SetPadding(0);

	check(WidgetInfo.IsValid());

	SMultiColumnTableRow<TSharedRef<FGASAbilityNode>>::Construct(SMultiColumnTableRow<TSharedRef<FGASAbilityNode>>::FArguments().Padding(0.f), InOwnerTableView);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASAbilityItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (SGASAbilitiesTab::AbilityNameColumn == ColumnName)
	{
		return CreateNameColumn();
	}
	else if (SGASAbilitiesTab::AbilityStateColumn == ColumnName)
	{
		return CreateStateColumn();
	}
	else if (SGASAbilitiesTab::AbilityActiveStateColumn == ColumnName)
	{
		return CreateActiveStateColumn();
	}
	else if (SGASAbilitiesTab::AbilityTriggersColumn == ColumnName)
	{
		return CreateTriggersColumn();
	}

	return SNullWidget::NullWidget;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASAbilityItem::CreateNameColumn()
{
	TSharedPtr<SWidget> NameWidgetBlock;
	if (WidgetInfo->CanNavigateToSource())
	{
		NameWidgetBlock =
			SNew(SHyperlink)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetName))
			.HighlightText(HighlightText)
			.OnNavigate(this, &SGASAbilityItem::HandleHyperlinkNavigate);
	}
	else
	{
		NameWidgetBlock =
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetName))
			.HighlightText(HighlightText);
	}

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
		.AutoWidth()
		.Padding(2.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.BorderBackgroundColor(FLinearColor::Transparent)
			.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetColor))
			[
				NameWidgetBlock ? NameWidgetBlock.ToSharedRef() : SNullWidget::NullWidget
			]
		];
}

TSharedRef<SWidget> SGASAbilityItem::CreateStateColumn() const
{
	return
		SNew(SBorder)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetColor))
		[
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetState))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> SGASAbilityItem::CreateActiveStateColumn() const
{
	return
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetColor))
		[
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetActiveState))
			.Justification(ETextJustify::Center)
		];
}

TSharedRef<SWidget> SGASAbilityItem::CreateTriggersColumn() const
{
	return
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetColor))
		[
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetTriggersData))
			.ToolTipText(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetTriggersData))
			.Justification(ETextJustify::Center)
		];
}

void SGASAbilityItem::HandleHyperlinkNavigate() const
{
	WidgetInfo->NavigateToSource();
}

#undef LOCTEXT_NAMESPACE