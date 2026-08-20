// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SHeaderRow.h"

struct FGASAttachEditorSettings
{
public:
	static void LoadNameSet(const TCHAR* Key, TSet<FName>& OutValue);
	static void SaveNameSet(const TCHAR* Key, const TSet<FName>& Value);

	static bool LoadBool(const TCHAR* Key, bool bDefaultValue);
	static void SaveBool(const TCHAR* Key, bool bValue);

	static int32 LoadInt(const TCHAR* Key, int32 DefaultValue);
	static void SaveInt(const TCHAR* Key, int32 Value);

	static EColumnSortMode::Type LoadSortMode(const TCHAR* Key);
	static void SaveSortMode(const TCHAR* Key, EColumnSortMode::Type Value);

private:
	static const TCHAR* GetSection();
};