// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GASAttachEditorStyle.h"
#include "Framework/Commands/Commands.h"

class FGASAttachEditorCommands : public TCommands<FGASAttachEditorCommands>
{
public:
	FGASAttachEditorCommands()
		: TCommands<FGASAttachEditorCommands>(TEXT("GASAttachEditor"), NSLOCTEXT("Contexts", "GASAttachEditor", "Debug Ability System"), NAME_None, FGASAttachEditorStyle::GetStyleName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > ShowGASAttachEditorViewer;

#if WITH_EDITOR
	TSharedPtr< FUICommandInfo > ShowGASTagLookAssetViewer;
#endif
};