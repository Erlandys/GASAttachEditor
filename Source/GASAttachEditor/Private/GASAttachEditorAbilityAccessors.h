// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"

class UGameplayTask;

/**
 * Where a runtime GAS object came from, so a row can jump to it.
 *
 * Always resolved from the CLASS, never from the object: an instanced ability lives inside the PIE
 * world package, so its object path points at the world rather than at any asset on disk. Resolve
 * once and keep it - it stays usable after PIE ends, when the runtime object is gone.
 *
 * Blueprint classes navigate to the asset editor; native classes navigate to the source file in the
 * configured IDE.
 */
struct FGASSourceAsset
{
public:
	static FGASSourceAsset FromObject(const UObject* Object);
	static FGASSourceAsset FromClass(const UClass* Class);

	bool IsValid() const;
	bool CanNavigate() const;
	void Navigate() const;

private:
	// Set for Blueprint-backed classes - a soft path so it outlives the asset being unloaded
	FSoftObjectPath AssetPath;

	// Set for native classes - native UClasses are rooted, so this stays valid for the session
	TWeakObjectPtr<const UClass> NativeClass;
};

struct FGASAbilityAccessors
{
public:
	static FName GetAbilityTriggersPropertyName();
	static FName GetActiveTasksPropertyName();
	static FName GetActivationOwnedTagsPropertyName();
	static FName GetActivationBlockedTagsPropertyName();

	static const TArray<FAbilityTriggerData>* FindAbilityTriggers(const UGameplayAbility* Ability);
	static const TArray<TObjectPtr<UGameplayTask>>* FindActiveTasks(const UGameplayAbility* Ability);

private:
	template<typename ValueType>
	static const ValueType* FindArrayProperty(const UGameplayAbility* Ability, const FName PropertyName)
	{
		if (!Ability)
		{
			return nullptr;
		}

		const FArrayProperty* Property = FindFProperty<FArrayProperty>(Ability->GetClass(), PropertyName);
		if (!Property)
		{
			return nullptr;
		}

		return Property->ContainerPtrToValuePtr<ValueType>(Ability);
	}
};