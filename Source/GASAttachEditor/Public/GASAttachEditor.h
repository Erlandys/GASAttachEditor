// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"

class SWidget;
class SDockTab;
class FMenuBuilder;
class FUICommandList;
class FToolBarBuilder;

class FGASAttachEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> OnSpawnGASEditorTab(const FSpawnTabArgs& SpawnTabArgs);

#if WITH_EDITOR
	TSharedRef<SDockTab> OnSpawnGASTriggersTab(const FSpawnTabArgs& SpawnTabArgs);
#endif

private:
	TSharedPtr<FUICommandList> PluginCommands;
	TWeakPtr<SDockTab> GASEditorTab;
	TWeakPtr<SDockTab> GASTriggersTab;
};