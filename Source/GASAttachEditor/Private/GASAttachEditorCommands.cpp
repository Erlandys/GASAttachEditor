// Copyright Epic Games, Inc. All Rights Reserved.

#include "GASAttachEditorCommands.h"

#define LOCTEXT_NAMESPACE "FGASAttachEditorModule"

void FGASAttachEditorCommands::RegisterCommands()
{
	UI_COMMAND(ShowGASAttachEditorViewer, "Ability System Debug", "Bring up Ability System Debug window", EUserInterfaceActionType::Check, FInputChord());
#if WITH_EDITOR
	UI_COMMAND(ShowGASTagLookAssetViewer, "Ability Triggers", "Bring up Ability Triggers window", EUserInterfaceActionType::Check, FInputChord());
#endif
}

#undef LOCTEXT_NAMESPACE
