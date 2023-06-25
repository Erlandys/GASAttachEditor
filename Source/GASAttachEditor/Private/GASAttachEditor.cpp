// Copyright Epic Games, Inc. All Rights Reserved.

#include "GASAttachEditor.h"

#include "Widgets/SGASEditorWidget.h"
#include "GASAttachEditorCommands.h"
#include "Widgets/SGASTriggersWidget.h"

#if WITH_EDITOR
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#endif

#include "Widgets/Docking/SDockTab.h"


static const FName GASAttachEditorTabName("GASAttachEditor");
static const FName GASTriggersEditorTabName("GASTriggersEditor");

#define LOCTEXT_NAMESPACE "FGASAttachEditorModule"

void FGASAttachEditorModule::StartupModule()
{
	FGASAttachEditorStyle::Initialize();
	FGASAttachEditorStyle::ReloadTextures();

	FGASAttachEditorCommands::Register();

	PluginCommands = MakeShared<FUICommandList>();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GASAttachEditorTabName, FOnSpawnTab::CreateRaw(this, &FGASAttachEditorModule::OnSpawnGASEditorTab))
		.SetDisplayName(LOCTEXT("FGASAttachEditorTabTitle", "Ability System Viewer"))
		.SetTooltipText(LOCTEXT("FGASAttachEditorTooltipText", "Opens 'Ability System Viewer' tab"))
#if WITH_EDITOR
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
#endif
		.SetIcon(FSlateIcon(FGASAttachEditorStyle::GetStyleName(), "GASAttachEditor.OpenPluginWindow"));


#if WITH_EDITOR
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GASTriggersEditorTabName, FOnSpawnTab::CreateRaw(this, &FGASAttachEditorModule::OnSpawnGASTriggersTab))
		.SetDisplayName(LOCTEXT("FGASTriggersEditorTabTitle", "Ability System Triggers Viewer"))
		.SetTooltipText(LOCTEXT("FGASTriggersEditorTooltipText", "Opens 'Ability System Triggers Viewer' tab"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
		.SetIcon(FSlateIcon(FGASAttachEditorStyle::GetStyleName(), "GASAttachEditor.OpenPluginWindow"));
#endif
}

void FGASAttachEditorModule::ShutdownModule()
{
#if WITH_EDITOR
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);
#endif

	FGASAttachEditorStyle::Shutdown();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GASAttachEditorTabName);
#if WITH_EDITOR
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GASTriggersEditorTabName);
#endif

	if (GASEditorTabManager.IsValid())
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(GASAttachEditorTabName);
#if WITH_EDITOR
		FGlobalTabmanager::Get()->UnregisterTabSpawner(GASTriggersEditorTabName);
#endif

		GASEditorTabLayout.Reset();
		GASEditorTabManager.Reset();
	}

	FGASAttachEditorCommands::Unregister();
}

TSharedRef<SDockTab> FGASAttachEditorModule::OnSpawnGASEditorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> NomadTab = SAssignNew(GASEditorTab, SDockTab).TabRole(ETabRole::NomadTab);

	NomadTab->SetContent(
		SNew(SBorder)
#if WITH_EDITOR
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
#endif
		.Padding(0.f, 2.f)
		[
			SNew(SGASEditorWidget)
			.ParentTab(NomadTab)
		]
	);

	return NomadTab;
}

#if WITH_EDITOR
TSharedRef<SDockTab> FGASAttachEditorModule::OnSpawnGASTriggersTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> NomadTab = SAssignNew(GASTriggersTab, SDockTab).TabRole(ETabRole::NomadTab);

	NomadTab->SetContent(
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(0.f, 2.f)
		[
			SNew(SGASTriggersWidget)
		]
	);

	return NomadTab;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void GASAttachEditorShow(UWorld* InWorld)
{
	FGlobalTabmanager::Get()->TryInvokeTab(GASAttachEditorTabName);
}

FAutoConsoleCommandWithWorld AbilitySystemEditorShow(
	TEXT("GASAttachEditor.Show"),
	TEXT("Open the editor for viewing actors Gameplay Abilities Data"),
	FConsoleCommandWithWorldDelegate::CreateStatic(GASAttachEditorShow)
);

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGASAttachEditorModule, GASAttachEditor)