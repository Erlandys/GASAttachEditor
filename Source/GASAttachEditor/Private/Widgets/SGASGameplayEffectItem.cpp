#include "SGASGameplayEffectItem.h"

#include "AbilitySystemComponent.h"
#include "Widgets/SGASGameplayEffectsTab.h"

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

void FGASGameplayEffectNodeBase::Update()
{
	Name = GatherName();
	DurationText = GatherDuration();
	StackText = GatherStack();
	LevelText = GatherLevel();
	Prediction = GatherPrediction();
	GrantedTagsText = GatherGrantedTags();

	CreateChildren();
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

	FText Result = LOCTEXT("GameplayEffectInfiniteDuration", "Inifinite Duration");

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

	if (GameplayEffect->Spec.Def->StackingType == EGameplayEffectStackingType::AggregateBySource)
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

	return LOCTEXT("GameplayEffectPredictedCought", "Predicted and Cought Up");
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
				SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(2.f, 0.f)
				[
					SNew(STextBlock)
					.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetName))
				]
			];
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectDurationColumn)
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetDurationText))
			];
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectStackColumn)
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetStackText))
			];
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectLevelColumn)
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetLevelText))
				.Justification(ETextJustify::Center)
			];
	}

	if (ColumnName == SGASGameplayEffectsTab::GameplayEffectGrantedTagsColumn)
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetGrantedTags))
				.ToolTipText(MakeAttributeSP(WidgetInfo.Get(), &FGASGameplayEffectNodeBase::GetGrantedTags))
			];
	}

	return SNullWidget::NullWidget;
}

#undef LOCTEXT_NAMESPACE

