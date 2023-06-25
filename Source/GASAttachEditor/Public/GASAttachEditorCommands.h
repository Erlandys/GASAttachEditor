// Copyright Epic Games, Inc. All Rights Reserved.

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