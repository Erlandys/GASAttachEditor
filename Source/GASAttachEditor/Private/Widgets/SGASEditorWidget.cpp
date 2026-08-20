// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASEditorWidget.h"

#include "SGASAbilitiesTab.h"
#include "SGASAttributesTab.h"
#include "SGASGameplayTagsTab.h"
#include "SGASGameplayEffectsTab.h"
#include "GASAttachEditorSettings.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Widgets/Input/SButton.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SCheckBox.h"
#include "GameFramework/Controller.h"
#include "Widgets/Docking/SDockTab.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Input/SComboButton.h"
#include "GameFramework/PlayerController.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Selection.h"
#include "UnrealEdMisc.h"
#include "Framework/Docking/LayoutService.h"
#endif

#define LOCTEXT_NAMESPACE "GASAttachEditor"

const TCHAR* SGASEditorWidget::ContinuousUpdateKey = TEXT("ContinuousUpdate");
const TCHAR* SGASEditorWidget::TrackSelectionKey = TEXT("TrackSelection");

static const FName AbilitiesTabName = "SGASEditor.AbilitiesTab";
static const FName AttributesTabName = "SGASEditor.AttributesTab";
static const FName GameplayEffectsTabName = "SGASEditor.GameplayEffectsTab";
static const FName GameplayTagsTabName = "SGASEditor.GameplayTagsTab";

SGASEditorWidget::~SGASEditorWidget()
{
#if WITH_EDITOR
	if (SelectionChangedHandle.IsValid())
	{
		USelection::SelectionChangedEvent.Remove(SelectionChangedHandle);
	}
#endif
}

void SGASEditorWidget::Construct(const FArguments& InArgs)
{
	bContinuousUpdate = FGASAttachEditorSettings::LoadBool(ContinuousUpdateKey, false);
#if WITH_EDITOR
	bTrackSelection = FGASAttachEditorSettings::LoadBool(TrackSelectionKey, false);
	SelectionChangedHandle = USelection::SelectionChangedEvent.AddSP(this, &SGASEditorWidget::HandleEditorSelectionChanged);
#endif

	CreateTabManager(InArgs._ParentTab);

	SelectedWorldTitle = LOCTEXT("None", "None");
	SelectedComponentTitle = LOCTEXT("None", "None");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(2.f, 2.f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("World", "World: "))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.MinDesiredWidth(125.f)
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SGASEditorWidget::OnGetWorldTypes)
					.VAlign(VAlign_Center)
					.ContentPadding(2.f)
					.IsEnabled_Lambda([this]
					{
						return !bTrackSelection;
					})
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText_Lambda([this]
						{
							return bTrackSelection
								? LOCTEXT("WorldTrackedToolTip", "Driven by Track Selected Object - turn it off to pick a world by hand")
								: LOCTEXT("WorldToolTip", "World for actors selection");
						})
						.Text_Lambda([this]
						{
							return SelectedWorldTitle;
						})
					]
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.f, 0.f, 0.f, 0.f)
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Actor", "Actor: "))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.MinDesiredWidth(125.f)
				[
					SNew(SComboButton)
					.OnGetMenuContent(this, &SGASEditorWidget::OnGetActorsList)
					.VAlign(VAlign_Center)
					.ContentPadding(2.f)
					.IsEnabled_Lambda([this]
					{
						return
							!bTrackSelection &&
							!SelectedWorldContextHandle.IsNone();
					})
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText_Lambda([this]
						{
							return bTrackSelection
								? LOCTEXT("ActorTrackedToolTip", "Driven by Track Selected Object - turn it off to pick an actor by hand")
								: LOCTEXT("ActorSelectionToolTip", "Actor selection");
						})
						.Text_Lambda([this]
						{
							return SelectedComponentTitle;
						})
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.Padding(2.f, 10.f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Left)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.OnClicked(this, &SGASEditorWidget::HandleRefreshClicked)
				.ToolTipText(LOCTEXT("RefreshToolTip", "Refreshes GAS data for selected world and actor"))
				.IsEnabled_Lambda([this]
				{
					return !bContinuousUpdate;
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5.f, 0.f)
			[
				SNew(SCheckBox)
				.Padding(FMargin(4.f, 0.f))
				.IsChecked_Lambda([this]
				{
					return bContinuousUpdate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState)
				{
					bContinuousUpdate = !bContinuousUpdate;
					FGASAttachEditorSettings::SaveBool(ContinuousUpdateKey, bContinuousUpdate);
				})
				[
					SNew(SBox)
					.MinDesiredWidth(125.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ContinuousUpdate", "Continuous Update"))
						.ToolTipText(LOCTEXT("ContinuousUpdateToolTip", "Re-read the selected component continuously instead of only when Refresh is pressed"))
					]
				]
			]
#if WITH_EDITOR
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(5.f, 0.f)
			[
				SNew(SCheckBox)
				.Padding(FMargin(4.f, 0.f))
				.IsChecked_Lambda([this]
				{
					return bTrackSelection ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState)
				{
					bTrackSelection = !bTrackSelection;
					FGASAttachEditorSettings::SaveBool(TrackSelectionKey, bTrackSelection);

					if (bTrackSelection &&
						GEditor)
					{
						HandleEditorSelectionChanged(GEditor->GetSelectedActors());
					}
				})
				[
					SNew(SBox)
					.MinDesiredWidth(125.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TrackSelection", "Track Selected Object"))
						.ToolTipText(LOCTEXT("TrackSelectionToolTip", "Follow the editor selection. Selecting a Pawn, Controller or Player State inspects whichever of them owns the Ability System Component."))
					]
				]
			]
#endif
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.BorderBackgroundColor(FLinearColor::Gray)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.Padding(0.f, 4.f, 0.f, 0.f)
				.VAlign(VAlign_Fill)
				[
					TabManager->RestoreFrom(GetLayout(), nullptr).ToSharedRef()
				]
			]
		]
	];
}

void SGASEditorWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Both of these are expensive - ValidateSelections scans every UAbilitySystemComponent in memory,
	// and a refresh re-gathers every row on every tab. Neither needs to happen at frame rate.
	if (InCurrentTime - LastUpdateTime < UpdateInterval)
	{
		return;
	}

	LastUpdateTime = InCurrentTime;

	ValidateSelections();

	if (bContinuousUpdate)
	{
		Refresh();
	}
}

void SGASEditorWidget::CreateTabManager(const TSharedPtr<SDockTab>& ParentTab)
{
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ParentTab.ToSharedRef());
	TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateLambda([](const TSharedRef<FTabManager::FLayout>& LayoutToSave)
	{
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
		if (FGlobalTabmanager::Get()->CanSavePersistentLayouts())
#else
		if (FUnrealEdMisc::Get().IsSavingLayoutOnClosedAllowed())
#endif
		{
			FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, LayoutToSave);
		}
#endif //WITH_EDITOR
	}));

	auto RegisterTrackedTabSpawner = [this](const FName& TabId, const FOnSpawnTab& OnSpawnTab) -> FTabSpawnerEntry&
	{
		return TabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateLambda([this, OnSpawnTab](const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>
		{
			TSharedRef<SDockTab> SpawnedTab = OnSpawnTab.Execute(Args);
			OnTabSpawned(Args.GetTabId().TabType, SpawnedTab);
			return SpawnedTab;
		}));
	};

	RegisterTrackedTabSpawner(AbilitiesTabName, FOnSpawnTab::CreateSP(this, &SGASEditorWidget::SpawnAbilitiesTab)).SetDisplayName(LOCTEXT("AbilitiesTabName", "Abilities"));
	RegisterTrackedTabSpawner(AttributesTabName, FOnSpawnTab::CreateSP(this, &SGASEditorWidget::SpawnAttributesTab)).SetDisplayName(LOCTEXT("AttributesTabName", "Attributes"));
	RegisterTrackedTabSpawner(GameplayEffectsTabName, FOnSpawnTab::CreateSP(this, &SGASEditorWidget::SpawnGameplayEffectsTab)).SetDisplayName(LOCTEXT("GameplayEffectsTabName", "Gameplay Effects"));
	RegisterTrackedTabSpawner(GameplayTagsTabName, FOnSpawnTab::CreateSP(this, &SGASEditorWidget::SpawnGameplayTagsTab)).SetDisplayName(LOCTEXT("GameplayTagsTabName", "Gameplay Tags"));
}

void SGASEditorWidget::OnTabSpawned(const FName& TabIdentifier, const TSharedRef<SDockTab>& SpawnedTab)
{
	TWeakPtr<SDockTab>* ExistingTab = SpawnedTabs.Find(TabIdentifier);
	if (!ExistingTab)
	{
		SpawnedTabs.Add(TabIdentifier, SpawnedTab);
		return;
	}

	check(!ExistingTab->IsValid());
	*ExistingTab = SpawnedTab;
}

TSharedRef<FTabManager::FLayout> SGASEditorWidget::GetLayout() const
{
	TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("SGASEditor_Layout_V1_Dev")
	->AddArea(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split(
			FTabManager::NewStack()
			->SetSizeCoefficient(1.f)
			->AddTab(AbilitiesTabName, ETabState::OpenedTab)
			->AddTab(AttributesTabName, ETabState::OpenedTab)
			->AddTab(GameplayEffectsTabName, ETabState::OpenedTab)
			->AddTab(GameplayTagsTabName, ETabState::OpenedTab)
			->SetForegroundTab(AbilitiesTabName)
		)
	);

#if WITH_EDITOR
	if (GIsEditor)
	{
		Layout = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, Layout);
	}
#endif

	return Layout;
}

TSharedRef<SDockTab> SGASEditorWidget::SpawnAbilitiesTab(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.Label(LOCTEXT("AbilitiesTabName", "Abilities"))
		.ShouldAutosize(false)
		.CanEverClose(false)
		[
			SAssignNew(AbilitiesTab, SGASAbilitiesTab)
		];
}

TSharedRef<SDockTab> SGASEditorWidget::SpawnAttributesTab(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.Label(LOCTEXT("AttributesTabName", "Attributes"))
		.ShouldAutosize(false)
		.CanEverClose(false)
		[
			SAssignNew(AttributesTab, SGASAttributesTab)
		];
}

TSharedRef<SDockTab> SGASEditorWidget::SpawnGameplayEffectsTab(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.Label(LOCTEXT("GameplayEffectsTabName", "Gameplay Effects"))
		.ShouldAutosize(false)
		.CanEverClose(false)
		[
			SAssignNew(GameplayEffectsTab, SGASGameplayEffectsTab)
		];
}

TSharedRef<SDockTab> SGASEditorWidget::SpawnGameplayTagsTab(const FSpawnTabArgs& Args)
{
	return
		SNew(SDockTab)
		.Label(LOCTEXT("GameplayTagsTabName", "Gameplay Tags"))
		.ShouldAutosize(false)
		.CanEverClose(false)
		[
			SAssignNew(GameplayTagsTab, SGASGameplayTagsTab)
		];
}

void SGASEditorWidget::SelectLocallyControlledComponent()
{
	for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
	{
		UAbilitySystemComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		const AActor* Owner = Component->GetOwnerActor();

		if (const APawn* Character = Cast<APawn>(Owner))
		{
			if (Character->IsLocallyControlled())
			{
				OnChangeSelectedActor(Component);
				return;
			}
		}

		if (const APlayerController* Controller = Cast<APlayerController>(Owner))
		{
			if (Controller->IsLocalController())
			{
				OnChangeSelectedActor(Component);
				return;
			}
		}

		if (const APlayerState* PlayerState = Cast<APlayerState>(Owner))
		{
			if (PlayerState->GetPlayerController() &&
				PlayerState->GetPlayerController()->IsLocalController())
			{
				OnChangeSelectedActor(Component);
				return;
			}
		}
	}

	OnChangeSelectedActor({});
}

#if WITH_EDITOR
UAbilitySystemComponent* SGASEditorWidget::FindAbilitySystemComponentChecked(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
}

UAbilitySystemComponent* SGASEditorWidget::FindRelatedAbilitySystemComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(Actor))
	{
		return Component;
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(Pawn->GetController()))
		{
			return Component;
		}

		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(Pawn->GetPlayerState()))
		{
			return Component;
		}
	}

	if (const AController* Controller = Cast<AController>(Actor))
	{
		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(Controller->GetPawn()))
		{
			return Component;
		}

		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(Controller->PlayerState))
		{
			return Component;
		}
	}

	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(PlayerState->GetOwningController()))
		{
			return Component;
		}

		if (UAbilitySystemComponent* Component = FindAbilitySystemComponentChecked(PlayerState->GetPawn()))
		{
			return Component;
		}
	}

	return nullptr;
}

void SGASEditorWidget::HandleEditorSelectionChanged(UObject* NewSelection)
{
	if (!bTrackSelection ||
		!GEditor)
	{
		return;
	}

	USelection* Selection = Cast<USelection>(NewSelection);
	if (!Selection)
	{
		return;
	}

	AActor* SelectedActor = Selection->GetTop<AActor>();
	if (!SelectedActor)
	{
		return;
	}

	if (AActor* Counterpart = EditorUtilities::GetSimWorldCounterpartActor(SelectedActor))
	{
		SelectedActor = Counterpart;
	}

	UAbilitySystemComponent* Component = FindRelatedAbilitySystemComponent(SelectedActor);
	if (!Component)
	{
		ClearSelection();
		return;
	}

	const UWorld* World = Component->GetWorld();
	if (!World)
	{
		ClearSelection();
		return;
	}

	if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
	{
		if (WorldContext->ContextHandle != SelectedWorldContextHandle)
		{
			OnChangeWorldType(WorldContext->ContextHandle);
		}
	}

	OnChangeSelectedActor(Component);
}
#endif

void SGASEditorWidget::ValidateSelections()
{
	if (SelectedWorldContextHandle.IsNone())
	{
		const TIndirectArray<FWorldContext>& Worlds = GEngine->GetWorldContexts();
		FName ValidWorld;
		for (const FWorldContext& WorldContext : Worlds)
		{
			if (WorldContext.WorldType != EWorldType::PIE &&
				WorldContext.WorldType != EWorldType::Game)
			{
				continue;
			}

			ValidWorld = WorldContext.ContextHandle;
			break;
		}

		if (!ValidWorld.IsNone())
		{
			OnChangeWorldType(ValidWorld);
		}
		return;
	}

	if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromHandle(SelectedWorldContextHandle))
	{
		if (const UWorld* World = WorldContext->World())
		{
			UpdateComponentsList(World);

			if (bTrackSelection)
			{
				return;
			}

			if (!SelectedComponent.IsValid() ||
				!AbilitySystemComponents.Contains(SelectedComponent))
			{
				SelectLocallyControlledComponent();
			}
			return;
		}
	}

	OnChangeWorldType({});
	OnChangeSelectedActor({});
}

void SGASEditorWidget::ClearSelection()
{
	bSelectionStopped = false;
	SelectedComponent = nullptr;
	SelectedComponentTitle = LOCTEXT("None", "None");

	AbilitiesTab->Refresh(nullptr);
	AttributesTab->Refresh(nullptr);
	GameplayEffectsTab->Refresh(nullptr, SelectedWorldContextHandle);
	GameplayTagsTab->Refresh(nullptr);
}

void SGASEditorWidget::Refresh()
{
	UAbilitySystemComponent* Component = SelectedComponent.Get();
	if (!Component)
	{
		return;
	}

	AbilitiesTab->Refresh(Component);
	AttributesTab->Refresh(Component);
	GameplayEffectsTab->Refresh(Component, SelectedWorldContextHandle);
	GameplayTagsTab->Refresh(Component);
}

FReply SGASEditorWidget::HandleRefreshClicked()
{
	Refresh();

	return FReply::Handled();
}

TSharedRef<SWidget> SGASEditorWidget::OnGetWorldTypes()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<FName> ServerHandles;
	TArray<FName> ClientHandles;
	
	const TIndirectArray<FWorldContext>& Worlds = GEngine->GetWorldContexts();
	for (const FWorldContext& WorldContext : Worlds)
	{
		if (WorldContext.WorldType != EWorldType::PIE &&
			WorldContext.WorldType != EWorldType::Game)
		{
			continue;
		}

		if (const UWorld* World = WorldContext.World())
		{
			switch (World->GetNetMode())
			{
			case NM_Client:
			case NM_Standalone: ClientHandles.Add(WorldContext.ContextHandle); break;
			case NM_DedicatedServer:
			case NM_ListenServer: ServerHandles.Add(WorldContext.ContextHandle); break;
			default: break;
			}
		}
	}

	MenuBuilder.BeginSection("Server", LOCTEXT("SectionServer", "Server"));
	{
		for (const FName ServerHandle : ServerHandles)
		{
			MenuBuilder.AddMenuEntry(
				GetWorldInstanceName(ServerHandle),
				FText(),
				{},
				FUIAction(FExecuteAction::CreateSP(this, &SGASEditorWidget::OnChangeWorldType, ServerHandle)));
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Clients", LOCTEXT("SectionClient", "Clients"));
	{
		for (const FName ClientHandle : ClientHandles)
		{
			MenuBuilder.AddMenuEntry(
				GetWorldInstanceName(ClientHandle),
				{},
				{},
				FUIAction(FExecuteAction::CreateSP(this, &SGASEditorWidget::OnChangeWorldType, ClientHandle)));
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SGASEditorWidget::OnChangeWorldType(const FName WorldContextHandle)
{
	SelectedWorldContextHandle = WorldContextHandle;
	SelectedWorldTitle = GetWorldInstanceName(WorldContextHandle);

	const FWorldContext* WorldContext = GEngine->GetWorldContextFromHandle(SelectedWorldContextHandle);
	if (!WorldContext)
	{
		return;
	}

	const UWorld* World = WorldContext->World();
	if (!World)
	{
		return;
	}

	UpdateComponentsList(World);

	// If standalone, we don't have connection between actors, so resetting to none
	if (World->GetNetMode() == NM_Standalone)
	{
		OnChangeSelectedActor({});
		return;
	}

	const UAbilitySystemComponent* CurrentComponent = SelectedComponent.Get();
	if (!CurrentComponent)
	{
		SelectLocallyControlledComponent();
		return;
	}

	const AActor* CurrentOwner = CurrentComponent->GetOwnerActor();
	if (!CurrentOwner)
	{
		OnChangeSelectedActor({});
		return;
	}

	for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
	{
		UAbilitySystemComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		const AActor* Owner = Component->GetOwnerActor();
		if (Owner &&
			Owner->GetFName() == CurrentOwner->GetFName())
		{
			OnChangeSelectedActor(Component);
			return;
		}
	}

	// We haven't found connected actors, reset to none
	OnChangeSelectedActor({});
}

TSharedRef<SWidget> SGASEditorWidget::OnGetActorsList()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
	{
		const UAbilitySystemComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		MenuBuilder.AddMenuEntry(
			GetComponentName(Component),
			FText(),
			{},
			FUIAction(FExecuteAction::CreateSP(this, &SGASEditorWidget::OnChangeSelectedActor, WeakComponent)));
	}

	return MenuBuilder.MakeWidget();
}

void SGASEditorWidget::OnChangeSelectedActor(TWeakObjectPtr<UAbilitySystemComponent> WeakComponent)
{
	UAbilitySystemComponent* Component = WeakComponent.Get();
	if (!Component)
	{
		if (SelectedComponent.IsValid())
		{
			bSelectionStopped = true;
			SelectedComponentTitle = FText::Format(LOCTEXT("ComponentStoppedFormat", "{0} (stopped)"), SelectedComponentTitle);
		}
		else if (!bSelectionStopped)
		{
			SelectedComponentTitle = LOCTEXT("None", "None");
		}

		SelectedComponent = nullptr;
		return;
	}

	bSelectionStopped = false;
	SelectedComponent = Component;
	SelectedComponentTitle = GetComponentName(Component);

	Refresh();
}

void SGASEditorWidget::UpdateComponentsList(const UWorld* World)
{
	AbilitySystemComponents.Reset();

	for (UAbilitySystemComponent* Component : TObjectRange<UAbilitySystemComponent>())
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetWorld() != World)
		{
			continue;
		}

		AbilitySystemComponents.Add(Component);
	}
}

FText SGASEditorWidget::GetComponentName(const UAbilitySystemComponent* Component) const
{
	const auto GetLocalRoleText = [](const ENetRole Role) -> FText
	{
		switch (Role)
		{
		default: return FText::GetEmpty();
		case ROLE_SimulatedProxy: return LOCTEXT("SimulatedProxy", "Simulated Proxy");
		case ROLE_AutonomousProxy: return LOCTEXT("AutonomousProxy", "Autonomous Proxy");
		case ROLE_Authority: return LOCTEXT("Authority", "Authority");
		}
	};

	const AActor* Avatar = Component->GetAvatarActor_Direct();
	const AActor* Owner = Component->GetOwnerActor();
	if (!Avatar &&
		!Owner)
	{
		return LOCTEXT("None", "None");
	}

	const AActor* Target = Avatar ? Avatar : Owner;

	const FText Name = FText::FromString(Target->GetActorNameOrLabel());

	// We don't want to show local roles for standalone worlds
	if (Component->GetWorld()->GetNetMode() == NM_Standalone)
	{
		return Name;
	}

	const FText Role = GetLocalRoleText(Target->GetLocalRole());
	return FText::Format(LOCTEXT("ComponentNameWithRoleFormat", "{0} [{1}]"), Name, Role);
}

FText SGASEditorWidget::GetWorldInstanceName(const FName WorldContextHandle) const
{
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromHandle(WorldContextHandle);
	if (!WorldContext)
	{
		return LOCTEXT("None", "None");
	}

	const UWorld* World = WorldContext->World();
	if (!World)
	{
		return LOCTEXT("None", "None");
	}

	switch (World->GetNetMode())
	{
	case NM_Standalone: return FText::Format(LOCTEXT("StandaloneFormat", "{0} [{1}]"), LOCTEXT("Standalone", "Standalone"), FText::AsNumber(WorldContext->PIEInstance));
	case NM_DedicatedServer: return LOCTEXT("DedicatedServer", "Dedicated Server");
	case NM_ListenServer: return LOCTEXT("ListenServer", "Listen Server");
	case NM_Client: return FText::Format(LOCTEXT("ClientFormat", "{0} [{1}]"), LOCTEXT("Client", "Client"), FText::AsNumber(WorldContext->PIEInstance));
	default: return LOCTEXT("None", "None");
	}
}

#undef LOCTEXT_NAMESPACE