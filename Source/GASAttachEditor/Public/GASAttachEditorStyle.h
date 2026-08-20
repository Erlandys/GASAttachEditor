// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FGASAttachEditorStyle : public FSlateStyleSet
{
public:
	using FSlateStyleSet::FSlateStyleSet;

	static void Initialize();
	static void Shutdown();

	static void ReloadTextures();

	static const ISlateStyle& Get();

	static FName GetStyleName();

private:
	static TSharedRef<FGASAttachEditorStyle> Create();
	void Register();

private:
	static TSharedPtr<FGASAttachEditorStyle> StyleInstance;
};