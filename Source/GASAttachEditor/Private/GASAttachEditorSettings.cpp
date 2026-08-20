// Fill out your copyright notice in the Description page of Project Settings.

#include "GASAttachEditorSettings.h"

#include "Misc/ConfigCacheIni.h"

const TCHAR* FGASAttachEditorSettings::GetSection()
{
	return TEXT("GASAttachEditor");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FGASAttachEditorSettings::LoadNameSet(const TCHAR* Key, TSet<FName>& OutValue)
{
	OutValue.Reset();

	if (!GConfig)
	{
		return;
	}

	TArray<FString> Values;
	GConfig->GetArray(GetSection(), Key, Values, GEditorPerProjectIni);

	for (const FString& Value : Values)
	{
		if (Value.IsEmpty())
		{
			continue;
		}

		OutValue.Add(*Value);
	}
}

void FGASAttachEditorSettings::SaveNameSet(const TCHAR* Key, const TSet<FName>& Value)
{
	if (!GConfig)
	{
		return;
	}

	TArray<FString> Values;
	Values.Reserve(Value.Num());
	for (const FName& Name : Value)
	{
		Values.Add(Name.ToString());
	}

	Values.Sort();

	GConfig->SetArray(GetSection(), Key, Values, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FGASAttachEditorSettings::LoadBool(const TCHAR* Key, const bool bDefaultValue)
{
	if (!GConfig)
	{
		return bDefaultValue;
	}

	bool bValue = bDefaultValue;
	if (!GConfig->GetBool(GetSection(), Key, bValue, GEditorPerProjectIni))
	{
		return bDefaultValue;
	}

	return bValue;
}

void FGASAttachEditorSettings::SaveBool(const TCHAR* Key, const bool bValue)
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetBool(GetSection(), Key, bValue, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FGASAttachEditorSettings::LoadInt(const TCHAR* Key, const int32 DefaultValue)
{
	if (!GConfig)
	{
		return DefaultValue;
	}

	int32 Value = DefaultValue;
	if (!GConfig->GetInt(GetSection(), Key, Value, GEditorPerProjectIni))
	{
		return DefaultValue;
	}

	return Value;
}

void FGASAttachEditorSettings::SaveInt(const TCHAR* Key, const int32 Value)
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetInt(GetSection(), Key, Value, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

EColumnSortMode::Type FGASAttachEditorSettings::LoadSortMode(const TCHAR* Key)
{
	if (!GConfig)
	{
		return EColumnSortMode::None;
	}

	FString Value;
	if (!GConfig->GetString(GetSection(), Key, Value, GEditorPerProjectIni))
	{
		return EColumnSortMode::None;
	}

	if (Value == TEXT("Ascending"))
	{
		return EColumnSortMode::Ascending;
	}

	if (Value == TEXT("Descending"))
	{
		return EColumnSortMode::Descending;
	}

	return EColumnSortMode::None;
}

void FGASAttachEditorSettings::SaveSortMode(const TCHAR* Key, const EColumnSortMode::Type Value)
{
	if (!GConfig)
	{
		return;
	}

	const TCHAR* ValueText = TEXT("None");
	switch (Value)
	{
	default: ensure(false); break;
	case EColumnSortMode::None: ValueText = TEXT("None"); break;
	case EColumnSortMode::Ascending: ValueText = TEXT("Ascending"); break;
	case EColumnSortMode::Descending: ValueText = TEXT("Descending"); break;
	}

	GConfig->SetString(GetSection(), Key, ValueText, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}