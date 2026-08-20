// Fill out your copyright notice in the Description page of Project Settings.

#include "GASAttachEditorCommands.h"

#define LOCTEXT_NAMESPACE "GASAttachEditor"

void FGASAttachEditorCommands::RegisterCommands()
{
	UI_COMMAND(ShowGASAttachEditorViewer, "Ability System Debug", "Bring up Ability System Debug window", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::G));
#if WITH_EDITOR
	UI_COMMAND(ShowGASTagLookAssetViewer, "Ability Triggers", "Bring up Ability Triggers window", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::T));
#endif
}

#undef LOCTEXT_NAMESPACE