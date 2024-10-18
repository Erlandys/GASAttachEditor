#include "SGASAbilityItem.h"

#include "Styling/StyleColors.h"
#include "AbilitySystemComponent.h"
#include "Widgets/Input/SHyperlink.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "SGASAttachEditor"

FGASAbilityNode::FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpec& AbilitySpec)
	: Type(EGAAbilityNode::Ability)
	, AbilitySpecPtr(AbilitySpec)
	, WeakComponent(ASC)
{
}

FGASAbilityNode::FGASAbilityNode(const TWeakObjectPtr<UAbilitySystemComponent>& ASC, const FGameplayAbilitySpec& AbilitySpec, const TWeakObjectPtr<UGameplayTask>& InGameplayTask)
	: Type(EGAAbilityNode::Task)
	, AbilitySpecPtr(AbilitySpec)
	, WeakComponent(ASC)
	, GameplayTask(InGameplayTask)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FGASAbilityNode::Update(const FGameplayAbilitySpec& AbilitySpec, const uint8 VisibleStates)
{
	AbilitySpecPtr = AbilitySpec;
	Name = FetchName();
	State = FetchState(StateType);
	TriggersData = FetchTriggersData();
	ActiveState = IsActive() ? NSLOCTEXT("WidgetReflectorNode ","WidgetClippingYes", "Yes") : NSLOCTEXT("WidgetReflectorNode ", "WidgetClippingNo", "No");
	UpdateVisibility(VisibleStates);
	FixupColor();

	FixupTasks(VisibleStates);
}

void FGASAbilityNode::UpdateVisibility(const uint8 VisibleStates)
{
	Visibility = (VisibleStates & StateType) == StateType ? EVisibility::Visible : EVisibility::Collapsed;
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

	switch (Type)
	{
	default: check(false);
	case EGAAbilityNode::Ability: return FText::FromString(Component->CleanupName(GetNameSafe(AbilitySpecPtr.Ability)));
	case EGAAbilityNode::Task: return FText::FromString(GameplayTask.IsValid() ? GameplayTask->GetDebugString() : "");
	}
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

	if (AbilitySpecPtr.IsActive())
	{
		OutStateType = EAbilityStateType::Active;

		return FText::Format(FText::FromString(TEXT("{0}: {1}")), LOCTEXT("ActiveCount", "Active Count"), AbilitySpecPtr.ActiveCount);
	}

	if (WeakComponent->IsAbilityInputBlocked(AbilitySpecPtr.InputID))
	{
		OutStateType = EAbilityStateType::Blocked;

		return LOCTEXT("InputBlocked", "Input Blocked");
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	if (WeakComponent->AreAbilityTagsBlocked(AbilitySpecPtr.Ability->GetAssetTags()))
#else
	if (WeakComponent->AreAbilityTagsBlocked(AbilitySpecPtr.Ability->AbilityTags))
#endif
	{
		FGameplayTagContainer BlockedAbility;
		WeakComponent->GetBlockedAbilityTags(BlockedAbility);

		OutStateType = EAbilityStateType::Blocked;

		return LOCTEXT("TagBlocked", "Blocked Tags");
	}

	FGameplayTagContainer FailureTags;
	if (!AbilitySpecPtr.Ability->CanActivateAbility(AbilitySpecPtr.Handle, WeakComponent->AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags))
	{
		OutStateType = EAbilityStateType::Blocked;

		const float Cooldown =  AbilitySpecPtr.Ability->GetCooldownTimeRemaining(WeakComponent->AbilityActorInfo.Get());
		if (Cooldown > 0.f)
		{
			return FText::Format(FText::FromString(TEXT("{0}, {1}: {2}s")), LOCTEXT("CantActivate","Can't Activate"), LOCTEXT("Cooldown", "Cooldown Time"), Cooldown);
		}

		return LOCTEXT("CantActivate", "Can't Activate");
	}

	return {};
}

FText FGASAbilityNode::FetchTriggersData() const
{
	if (!WeakComponent.IsValid() ||
		!AbilitySpecPtr.Ability)
	{
		return {};
	}

	// To properly check AbilityTriggers variable for changes
	class UDummyAbility : public UGameplayAbility
	{
		friend class FGASAbilityNode;
	};

	const FArrayProperty* ActiveTagsPtr = FindFProperty<FArrayProperty>(AbilitySpecPtr.Ability->GetClass(), GET_MEMBER_NAME_CHECKED(UDummyAbility, AbilityTriggers));
	if (!ensure(ActiveTagsPtr))
	{
		return {};
	}

	const TArray<FAbilityTriggerData>* ActivationTagsPtr = ActiveTagsPtr->ContainerPtrToValuePtr<TArray<FAbilityTriggerData>>(AbilitySpecPtr.Ability);
	if (!ensure(ActivationTagsPtr))
	{
		return {};
	}

	FString Text;
	const TArray<FAbilityTriggerData>& ActivationTags = *ActivationTagsPtr;
	for (int32 Index = 0; Index < ActivationTags.Num(); Index++)
	{
		const FAbilityTriggerData* Item = &ActivationTags[Index];

		Text += "Tag: (" + Item->TriggerTag.ToString() + "), Event: (" + UEnum::GetDisplayValueAsText(Item->TriggerSource).ToString() + ")\n";
	}

	Text.RemoveFromEnd("\n");

	return FText::FromString(Text);
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

void FGASAbilityNode::FixupTasks(const uint8 VisibleStates)
{
	if (!WeakComponent.IsValid() ||
		!AbilitySpecPtr.IsActive() ||
		Type != EGAAbilityNode::Ability)
	{
		MappedChildNodes = {};
		ChildNodes = {};
		return;
	}

	// To properly check AbilityTriggers variable for changes
	class UDummyAbility : public UGameplayAbility
	{
		friend class FGASAbilityNode;
	};

	TSet<int32> InactiveTasks;
	MappedChildNodes.GetKeys(InactiveTasks);

	TArray<UGameplayAbility*> Instances = AbilitySpecPtr.GetAbilityInstances();
	for (UGameplayAbility* Instance : Instances)
	{
		if (!ensure(Instance))
		{
			continue;
		}

		const FArrayProperty* Property = FindFProperty<FArrayProperty>(Instance->GetClass(), GET_MEMBER_NAME_CHECKED(UDummyAbility, ActiveTasks));
		if (!ensure(Property))
		{
			continue;
		}

		const TArray<UGameplayTask*>* ActiveTasksPtr = Property->ContainerPtrToValuePtr<TArray<UGameplayTask*>>(Instance);
		if (!ensure(ActiveTasksPtr))
		{
			continue;
		}

		TArray<UGameplayTask*> ActiveTasks = *ActiveTasksPtr;
		for (UGameplayTask* Item : ActiveTasks)
		{
			if (!ensure(Item))
			{
				continue;
			}

			int32 Key = GetTypeHash(Item);
			InactiveTasks.Remove(Key);

			if (const TSharedPtr<FGASAbilityNode>& Task = MappedChildNodes.FindRef(Key))
			{
				Task->GameplayTask = Item;
				Task->Update(AbilitySpecPtr, VisibleStates);
				continue;
			}

			TSharedRef<FGASAbilityNode> NewTask = MakeShared<FGASAbilityNode>(WeakComponent, AbilitySpecPtr, Item);
			NewTask->Update(AbilitySpecPtr, VisibleStates);
			MappedChildNodes.Add(Key, NewTask);
		}
	}

	for (const int32 InactiveTask : InactiveTasks)
	{
		MappedChildNodes.Remove(InactiveTask);
	}

	MappedChildNodes.GenerateValueArray(ChildNodes);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FGASAbilityNode::GetWidgetAssetData() const
{
	if (Type != EGAAbilityNode::Ability)
	{
		return {};
	}

	const UGameplayAbility* Ability = AbilitySpecPtr.Ability;
	if (!Ability ||
		Ability->GetClass()->IsNative())
	{
		return {};
	}

	const FSoftObjectPath ObjectPath(Ability);

	// Fixup path name, to open proper asset, instead of CDO asset
	FString PathName = ObjectPath.GetAssetName();
	PathName.RemoveFromStart("Default__");
	PathName.RemoveFromEnd("_C");

	return ObjectPath.GetLongPackageName() + "." + PathName;
}

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
	SetPadding(0);

	check(WidgetInfo.IsValid());

	SetVisibility(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetVisibility));

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
	if (WidgetInfo->HasValidWidgetAssetData())
	{
		NameWidgetBlock =
			SNew(SHyperlink)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetName))
			.OnNavigate(this, &SGASAbilityItem::HandleHyperlinkNavigate);
	}
	else
	{
		NameWidgetBlock =
			SNew(STextBlock)
			.Text(MakeAttributeSP(WidgetInfo.Get(), &FGASAbilityNode::GetName));
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
	if (!WidgetInfo->HasValidWidgetAssetData())
	{
		return;
	}

#if WITH_EDITOR
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(*WidgetInfo->GetWidgetAssetData()));

	if (!AssetData.IsValid())
	{
		return;
	}

	UObject* Object = AssetData.GetAsset();
	if (!ensure(Object))
	{
		return;
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
#endif
}

#undef LOCTEXT_NAMESPACE