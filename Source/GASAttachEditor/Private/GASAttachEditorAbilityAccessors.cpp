// Fill out your copyright notice in the Description page of Project Settings.

#include "GASAttachEditorAbilityAccessors.h"

#include "GameplayTask.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Editor.h"
#include "SourceCodeNavigation.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

class UGASAccessorDummyAbility : public UGameplayAbility
{
	friend struct FGASAbilityAccessors;
};

FGASSourceAsset FGASSourceAsset::FromObject(const UObject* Object)
{
	if (!Object)
	{
		return {};
	}

	return FromClass(Object->GetClass());
}

FGASSourceAsset FGASSourceAsset::FromClass(const UClass* Class)
{
	FGASSourceAsset Result;
	if (!Class)
	{
		return Result;
	}

	// Native classes have no asset - they navigate to source instead
	if (Class->IsNative())
	{
		Result.NativeClass = Class;
		return Result;
	}

#if WITH_EDITORONLY_DATA
	if (const UObject* GeneratedBy = Class->ClassGeneratedBy)
	{
		Result.AssetPath = FSoftObjectPath(GeneratedBy);
		return Result;
	}
#endif

	const UPackage* Package = Class->GetPackage();
	if (!Package)
	{
		return Result;
	}

	// Fall back to the generated-class naming convention: /Game/X/BP_A.BP_A_C -> /Game/X/BP_A.BP_A
	FString AssetName = Class->GetName();
	AssetName.RemoveFromEnd(TEXT("_C"));

	Result.AssetPath = FSoftObjectPath(Package->GetName() + TEXT(".") + AssetName);

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FGASSourceAsset::IsValid() const
{
	return
		!AssetPath.IsNull() ||
		NativeClass.IsValid();
}

bool FGASSourceAsset::CanNavigate() const
{
#if WITH_EDITOR
	if (!AssetPath.IsNull())
	{
		return true;
	}

	if (const UClass* Class = NativeClass.Get())
	{
		return FSourceCodeNavigation::CanNavigateToClass(Class);
	}
#endif

	return false;
}

void FGASSourceAsset::Navigate() const
{
#if WITH_EDITOR
	if (const UClass* Class = NativeClass.Get())
	{
		FSourceCodeNavigation::NavigateToClass(Class);
		return;
	}

	if (AssetPath.IsNull())
	{
		return;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(AssetPath);
	if (!AssetData.IsValid())
	{
		return;
	}

	UObject* Object = AssetData.GetAsset();
	if (!ensure(Object))
	{
		return;
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FGASAbilityAccessors::GetAbilityTriggersPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UGASAccessorDummyAbility, AbilityTriggers);
}

FName FGASAbilityAccessors::GetActiveTasksPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UGASAccessorDummyAbility, ActiveTasks);
}

FName FGASAbilityAccessors::GetActivationOwnedTagsPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UGASAccessorDummyAbility, ActivationOwnedTags);
}

FName FGASAbilityAccessors::GetActivationBlockedTagsPropertyName()
{
	return GET_MEMBER_NAME_CHECKED(UGASAccessorDummyAbility, ActivationBlockedTags);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const TArray<FAbilityTriggerData>* FGASAbilityAccessors::FindAbilityTriggers(const UGameplayAbility* Ability)
{
	return FindArrayProperty<TArray<FAbilityTriggerData>>(Ability, GetAbilityTriggersPropertyName());
}

const TArray<TObjectPtr<UGameplayTask>>* FGASAbilityAccessors::FindActiveTasks(const UGameplayAbility* Ability)
{
	return FindArrayProperty<TArray<TObjectPtr<UGameplayTask>>>(Ability, GetActiveTasksPropertyName());
}