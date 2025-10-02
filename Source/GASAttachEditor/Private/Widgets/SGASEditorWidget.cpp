// Fill out your copyright notice in the Description page of Project Settings.

#include "SGASEditorWidget.h"

#include "AbilitySystemComponent.h"
#include "SGASAbilitiesTab.h"
#include "SGASAttributesTab.h"
#include "SGASGameplayTagsTab.h"
#include "SGASGameplayEffectsTab.h"

#include "GameFramework/PlayerState.h"
#include "Widgets/Input/SComboButton.h"

#define LOCTEXT_NAMESPACE "SGASEditor"

static const FName AbilitiesTabName = "SGASEditor.AbilitiesTab";
static const FName AttributesTabName = "SGASEditor.AttributesTab";
static const FName GameplayEffectsTabName = "SGASEditor.GameplayEffectsTab";
static const FName GameplayTagsTabName = "SGASEditor.GameplayTagsTab";

void SGASEditorWidget::Construct(const FArguments& InArgs)
{
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
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("WorldToolTip", "World for actors selection"))
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
						return !SelectedWorldContextHandle.IsNone();
					})
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText(LOCTEXT("ActorSelectionToolTip", "Actor selection"))
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
				.OnClicked(this, &SGASEditorWidget::Refresh)
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
				})
				[
					SNew(SBox)
					.MinDesiredWidth(125.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ContinuousUpdate", "Continuous Update"))
						.ToolTipText_Lambda([this]
						{
							if (bContinuousUpdate)
							{
								return LOCTEXT("ContinuousUpdateStop", "To stop continuous update press 'END' key");
							}
							return LOCTEXT("ContinuousUpdateStart", "To start continuous update press 'END' key");
						})
					]
				]
			]
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
			if (!SelectedComponent.IsValid() ||
				!AbilitySystemComponents.Contains(SelectedComponent))
			{
				for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
				{
					UAbilitySystemComponent* Component = WeakComponent.Get();
					if (!Component)
					{
						continue;
					}

					if (const APawn* Character = Cast<APawn>(Component->GetOwnerActor()))
					{
						if (Character->IsLocallyControlled())
						{
							OnChangeSelectedActor(Component);
							return;
						}
					}

					if (const APlayerController* Controller = Cast<APlayerController>(Component->GetOwnerActor()))
					{
						if (Controller->IsLocalController())
						{
							OnChangeSelectedActor(Component);
							return;
						}
					}

					if (const APlayerState* PlayerState = Cast<APlayerState>(Component->GetOwnerActor()))
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
			return;
		}
	}

	OnChangeWorldType({});
	OnChangeSelectedActor({});
}

FReply SGASEditorWidget::Refresh() const
{
	UAbilitySystemComponent* Component = SelectedComponent.Get();

	AbilitiesTab->Refresh(Component);
	AttributesTab->Refresh(Component);
	GameplayEffectsTab->Refresh(Component, SelectedWorldContextHandle);
	GameplayTagsTab->Refresh(Component);

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
		for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
		{
			UAbilitySystemComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				continue;
			}

			if (const APawn* Character = Cast<APawn>(Component->GetOwnerActor()))
			{
				if (Character->IsLocallyControlled())
				{
					OnChangeSelectedActor(Component);
					return;
				}
			}

			if (const APlayerController* Controller = Cast<APlayerController>(Component->GetOwnerActor()))
			{
				if (Controller->IsLocalController())
				{
					OnChangeSelectedActor(Component);
					return;
				}
			}

			if (const APlayerState* PlayerState = Cast<APlayerState>(Component->GetOwnerActor()))
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
		return;
	}

	for (const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent : AbilitySystemComponents)
	{
		UAbilitySystemComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		if (Component->GetOwnerActor()->GetFName() == CurrentComponent->GetOwnerActor()->GetFName())
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
		SelectedComponent = nullptr;
		SelectedComponentTitle = LOCTEXT("None", "None");
		return;
	}

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
		default: return INVTEXT("");
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

	const FText Name = FText::FromString(Target->GetName());

	// We don't want to show local roles for standalone worlds
	if (Component->GetWorld()->GetNetMode() == NM_Standalone)
	{
		return Name;
	}

	const FText Role = GetLocalRoleText(Target->GetLocalRole());
	return FText::Format(INVTEXT("{0} [{1}]"), Name, Role);
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