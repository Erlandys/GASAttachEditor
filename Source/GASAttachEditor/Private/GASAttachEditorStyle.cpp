// Copyright Epic Games, Inc. All Rights Reserved.

#include "GASAttachEditorStyle.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"

TSharedPtr<FGASAttachEditorStyle> FGASAttachEditorStyle::StyleInstance = nullptr;

void FGASAttachEditorStyle::Initialize()
{
	if (StyleInstance.IsValid())
	{
		return;
	}

	StyleInstance = Create();
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FGASAttachEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FGASAttachEditorStyle::GetStyleName()
{
	static FName StyleSetName(TEXT("GASAttachEditorStyle"));
	return StyleSetName;
}

TSharedRef< FGASAttachEditorStyle > FGASAttachEditorStyle::Create()
{
	TSharedRef<FGASAttachEditorStyle> Style = MakeShared<FGASAttachEditorStyle>("GASAttachEditorStyle");
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("GASAttachEditor")->GetBaseDir() / TEXT("Resources"));
	Style->Register();
	return Style;
}

void FGASAttachEditorStyle::Register()
{
	Set("GASAttachEditor.OpenPluginWindow", new IMAGE_BRUSH("ButtonIcon_40x", CoreStyleConstants::Icon16x16));
}

void FGASAttachEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FGASAttachEditorStyle::Get()
{
	return *StyleInstance;
}