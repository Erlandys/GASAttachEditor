// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASGameplayEffectItem.h"
#include "Styling/StyleColors.h"
#include "AbilitySystemComponent.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/SGASGameplayEffectsTab.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "GASAttachEditor"

void FGASGameplayEffectNodeBase::Update()
{
	Name = GatherName();
	DurationText = GatherDuration();
	StackText = GatherStack();
	LevelText = GatherLevel();
	Prediction = GatherPrediction();
	GrantedTagsText = GatherGrantedTags();
	bIsBlocked = GatherBlocked();
	StateText = GatherState();
	StateType = GatherStateType();

	// Resolve once - after that it must survive the effect, the component and PIE itself
	if (!SourceAsset.IsValid())
	{
		SourceAsset = FGASSourceAsset::FromClass(GatherSourceAssetClass());
	}

	FixupColor();

	CreateChildren();
}

void FGASGameplayEffectNodeBase::FixupColor()
{
	Tint = bIsBlocked
		? FStyleColors::Error.GetSpecifiedColor()
		: FSlateColor::UseForeground().GetColor(FWidgetStyle());
}

const TArray<TSharedPtr<FGASGameplayEffectNodeBase>>& FGASGameplayEffectNodeBase::GetChildNodes() const
{
	return ChildNodes;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FGASGameplayEffectNode::FGASGameplayEffectNode(const FName WorldContextHandle, const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FActiveGameplayEffectHandle& GameplayEffectHandle)
	: WorldContextHandle(WorldContextHandle)
	, WeakComponent(WeakComponent)
	, GameplayEffectHandle(GameplayEffectHandle)
{
}

FText FGASGameplayEffectNode::GatherName() const
{
	UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return LOCTEXT("None", "None");
	}

	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	return FText::FromString(Component->CleanupName(GetNameSafe(GameplayEffect->Spec.Def)));
}

FText FGASGameplayEffectNode::GatherDuration() const
{
	const UWorld* World = GetWorld();
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!World ||
		!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	FText Result = LOCTEXT("GameplayEffectInfiniteDuration", "Infinite Duration");

	FNumberFormattingOptions NumberFormatOptions;
	NumberFormatOptions.MaximumFractionalDigits = 2;

	if (GameplayEffect->GetDuration() > 0.f)
	{
		Result = FText::Format(
			LOCTEXT("GameplayEffectDurationFormat", "Duration: {0}, Remaining: {1}"),
			FText::AsNumber(GameplayEffect->GetDuration(), &NumberFormatOptions),
			FText::AsNumber(GameplayEffect->GetTimeRemaining(World->GetTimeSeconds()), &NumberFormatOptions));
	}

	if (GameplayEffect->GetPeriod() > 0.f)
	{
		Result = FText::Format(
			LOCTEXT("GameplayEffectPeriodFormat", "{0}, Period: {1}"),
			Result,
			FText::AsNumber(GameplayEffect->GetPeriod(), &NumberFormatOptions));
	}

	return Result;
}

FText FGASGameplayEffectNode::GatherStack() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return {};
	}

	if (GameplayEffect->Spec.GetStackCount() <= 1)
	{
		return {};
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 7
	if (GameplayEffect->Spec.Def->GetStackingType() == EGameplayEffectStackingType::AggregateBySource)
#else
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (GameplayEffect->Spec.Def->StackingType == EGameplayEffectStackingType::AggregateBySource)
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
	{
		if (const UAbilitySystemComponent* Component = GameplayEffect->Spec.GetContext().GetInstigatorAbilitySystemComponent())
		{
			if (const AActor* Avatar = Component->GetAvatarActor())
			{
				return FText::Format(LOCTEXT("GameplayEffectStacksFrom", "Stacks: {0}, From: {1}"), GameplayEffect->Spec.GetStackCount(), FText::FromString(Avatar->GetName()));
			}
		}
	}

	return FText::Format(LOCTEXT("GameplayEffectStacks", "Stacks: {0}"), GameplayEffect->Spec.GetStackCount());
}

FText FGASGameplayEffectNode::GatherLevel() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	return FText::AsNumber(GameplayEffect->Spec.GetLevel());
}

FText FGASGameplayEffectNode::GatherPrediction() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	if (!GameplayEffect->PredictionKey.IsValidKey())
	{
		return {};
	}

	if (GameplayEffect->PredictionKey.WasLocallyGenerated())
	{
		return LOCTEXT("GameplayEffectPredictionGenerated", "Predicted and Waiting");
	}

	return LOCTEXT("GameplayEffectPredictedCaughtUp", "Predicted and Caught Up");
}

FText FGASGameplayEffectNode::GatherGrantedTags() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	return FText::FromString(GrantedTags.ToStringSimple());
}

bool FGASGameplayEffectNode::GatherBlocked() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return false;
	}

	return GameplayEffect->bIsInhibited;
}

FText FGASGameplayEffectNode::GatherState() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return {};
	}

	if (GameplayEffect->bIsInhibited)
	{
		return LOCTEXT("GameplayEffectBlocked", "Blocked");
	}

	return LOCTEXT("GameplayEffectActive", "Active");
}

EGameplayEffectStateType::Type FGASGameplayEffectNode::GatherStateType() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return EGameplayEffectStateType::Active;
	}

	if (GameplayEffect->bIsInhibited)
	{
		return EGameplayEffectStateType::Inhibited;
	}

	if (GameplayEffect->GetDuration() <= 0.f)
	{
		return EGameplayEffectStateType::Infinite;
	}

	return EGameplayEffectStateType::Active;
}

const UClass* FGASGameplayEffectNode::GatherSourceAssetClass() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect ||
		!GameplayEffect->Spec.Def)
	{
		return nullptr;
	}

	return GameplayEffect->Spec.Def->GetClass();
}

void FGASGameplayEffectNode::CreateChildren()
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		ChildNodes.Reset();
		return;
	}

	if (!GameplayEffect->Spec.Def)
	{
		ChildNodes.Reset();
		return;
	}

	TSet<int32> InactiveModifiers;
	MappedModifiers.GetKeys(InactiveModifiers);

	for (int32 Index = 0; Index < GameplayEffect->Spec.Modifiers.Num(); ++Index)
	{
		if (!ensure(GameplayEffect->Spec.Def))
		{
			break;
		}

		if (!ensure(GameplayEffect->Spec.Modifiers.IsValidIndex(Index)) ||
			!ensure(GameplayEffect->Spec.Def->Modifiers.IsValidIndex(Index)))
		{
			continue;
		}

		InactiveModifiers.Remove(Index);

		if (const TSharedPtr<FGASGameplayEffectNodeBase>& ModifierNode = MappedModifiers.FindRef(Index))
		{
			ModifierNode->Update();
			continue;
		}

		TSharedRef<FGASGameplayEffectModifierNode> NewItem = MakeShared<FGASGameplayEffectModifierNode>(WeakComponent, GameplayEffectHandle, Index);
		NewItem->Update();
		MappedModifiers.Add(Index, NewItem);
	}

	for (const int32 InactiveModifierIndex : InactiveModifiers)
	{
		MappedModifiers.Remove(InactiveModifierIndex);
	}

	MappedModifiers.GenerateValueArray(ChildNodes);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UWorld* FGASGameplayEffectNode::GetWorld() const
{
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromHandle(WorldContextHandle);
	if (!WorldContext)
	{
		return nullptr;
	}

	return WorldContext->World();
}

const FActiveGameplayEffect* FGASGameplayEffectNode::GetGameplayEffect() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return nullptr;
	}

	return Component->GetActiveGameplayEffect(GameplayEffectHandle);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FGASGameplayEffectModifierNode::FGASGameplayEffectModifierNode(const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FActiveGameplayEffectHandle& GameplayEffectHandle, const int32 ModifierIndex)
	: WeakComponent(WeakComponent)
	, GameplayEffectHandle(GameplayEffectHandle)
	, ModifierIndex(ModifierIndex)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText FGASGameplayEffectModifierNode::GatherName() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	const FGameplayModifierInfo* ModifierInfo = GetModifierInfo(GameplayEffect);
	if (!ensure(ModifierInfo))
	{
		return LOCTEXT("None", "None");
	}

	return FText::FromString(ModifierInfo->Attribute.AttributeName);
}

FText FGASGameplayEffectModifierNode::GatherDuration() const
{
	const FActiveGameplayEffect* GameplayEffect = GetGameplayEffect();
	if (!GameplayEffect)
	{
		return LOCTEXT("None", "None");
	}

	const FModifierSpec* ModifierSpec = GetModifierSpec(GameplayEffect);
	const FGameplayModifierInfo* ModifierInfo = GetModifierInfo(GameplayEffect);
	if (!ensure(ModifierInfo) ||
		!ensure(ModifierSpec))
	{
		return LOCTEXT("None", "None");
	}

	const UEnum* Enum = StaticEnum<EGameplayModOp::Type>();
	return
		FText::Format(
			LOCTEXT("GameplayEffectModifier", "Modifier: {0}, Value: {1}"),
			FText::FromString(Enum->GetNameStringByValue(ModifierInfo->ModifierOp)),
			ModifierSpec->GetEvaluatedMagnitude());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FActiveGameplayEffect* FGASGameplayEffectModifierNode::GetGameplayEffect() const
{
	const UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		return nullptr;
	}

	return Component->GetActiveGameplayEffect(GameplayEffectHandle);
}

const FModifierSpec* FGASGameplayEffectModifierNode::GetModifierSpec(const FActiveGameplayEffect* GameplayEffect) const
{
	if (!ensure(GameplayEffect->Spec.Modifiers.IsValidIndex(ModifierIndex)))
	{
		return nullptr;
	}

	return &GameplayEffect->Spec.Modifiers[ModifierIndex];
}

const FGameplayModifierInfo* FGASGameplayEffectModifierNode::GetModifierInfo(const FActiveGameplayEffect* GameplayActiveEffect) const
{
	const UGameplayEffect* GameplayEffect = GameplayActiveEffect->Spec.Def;
	if (!ensure(GameplayEffect))
	{
		return nullptr;
	}

	if (!ensure(GameplayEffect->Modifiers.IsValidIndex(ModifierIndex)))
	{
		return nullptr;
	}

	return &GameplayEffect->Modifiers[ModifierIndex];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SGASGameplayEffectTreeItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	WidgetInfo = InArgs._WidgetInfoToVisualize;
	HighlightText = InArgs._HighlightText;
	SetPadding(0.f);

	check(WidgetInfo.IsValid());

	SMultiColumnTableRow<TSharedPtr<FGASGameplayEffectNodeBase>>::Construct(SMultiColumnTableRow<TSharedPtr<FGASGameplayEffectNodeBase>>::FArguments().Padding(0.f), InOwnerTableView);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASGameplayEffectTreeItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectNameColumn)
	{
		return CreateNameColumn();
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectStateColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetState),
				HAlign_Left,
				ETextJustify::Center);
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectDurationColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetDurationText),
				HAlign_Left,
				ETextJustify::Left);
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectStackColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetStackText),
				HAlign_Left,
				ETextJustify::Left);
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectLevelColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetLevelText),
				HAlign_Center,
				ETextJustify::Center);
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectPredictionColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetPrediction),
				HAlign_Center,
				ETextJustify::Center);
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectGrantedTagsColumn)
	{
		return
			CreateTextColumn(
				MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetGrantedTags),
				HAlign_Left,
				ETextJustify::Left,
				true);
	}

	return SNullWidget::NullWidget;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASGameplayEffectTreeItem::CreateTextColumn(
	const TAttribute<FText>& Text,
	const EHorizontalAlignment HorizontalAlignment,
	const ETextJustify::Type Justification,
	const bool bShowToolTip) const
{
	return
		SNew(SBorder)
		.HAlign(HorizontalAlignment)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetColor))
		[
			SNew(STextBlock)
			.Text(Text)
			.HighlightText(HighlightText)
			.ToolTipText(bShowToolTip ? Text : TAttribute<FText>())
			.Justification(Justification)
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SGASGameplayEffectTreeItem::CreateNameColumn()
{
	TSharedPtr<SWidget> NameWidgetBlock;
	if (WidgetInfo->CanNavigateToSource())
	{
		NameWidgetBlock =
			SNew(SHyperlink)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetName))
			.HighlightText(HighlightText)
			.OnNavigate(this, &SGASGameplayEffectTreeItem::HandleHyperlinkNavigate);
	}
	else
	{
		NameWidgetBlock =
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetName))
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
			.ColorAndOpacity(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetColor))
			[
				NameWidgetBlock ? NameWidgetBlock.ToSharedRef() : SNullWidget::NullWidget
			]
		];
}

void SGASGameplayEffectTreeItem::HandleHyperlinkNavigate() const
{
	WidgetInfo->NavigateToSource();
}

#undef LOCTEXT_NAMESPACE