// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Docking/TabManager.h"

class SGASAbilitiesTab;
class SGASAttributesTab;
class SGASGameplayTagsTab;
class SGASGameplayEffectsTab;
class UAbilitySystemComponent;

class SGASEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASEditorWidget)
	{}
		SLATE_ARGUMENT(TSharedPtr<SDockTab>, ParentTab)
	SLATE_END_ARGS()

	virtual ~SGASEditorWidget() override;

	void Construct(const FArguments& InArgs);

	//~ Begin SCompoundWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	//~ End SCompoundWidget Interface

private:
	void CreateTabManager(const TSharedPtr<SDockTab>& ParentTab);
	void OnTabSpawned(const FName& TabIdentifier, const TSharedRef<SDockTab>& SpawnedTab);
	TSharedRef<FTabManager::FLayout> GetLayout() const;

	TSharedRef<SDockTab> SpawnAbilitiesTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAttributesTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGameplayEffectsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGameplayTagsTab(const FSpawnTabArgs& Args);

	static const TCHAR* ContinuousUpdateKey;
	static const TCHAR* TrackSelectionKey;

#if WITH_EDITOR
	void HandleEditorSelectionChanged(UObject* NewSelection);
	static UAbilitySystemComponent* FindRelatedAbilitySystemComponent(AActor* Actor);
	static UAbilitySystemComponent* FindAbilitySystemComponentChecked(AActor* Actor);
#endif

	void ValidateSelections();
	void SelectLocallyControlledComponent();
	void ClearSelection();
	void Refresh();
	FReply HandleRefreshClicked();
	TSharedRef<SWidget> OnGetWorldTypes();
	void OnChangeWorldType(FName WorldContextHandle);
	TSharedRef<SWidget> OnGetActorsList();
	void OnChangeSelectedActor(TWeakObjectPtr<UAbilitySystemComponent> WeakComponent);
	void UpdateComponentsList(const UWorld* World);

	FText GetComponentName(const UAbilitySystemComponent* Component) const;
	FText GetWorldInstanceName(FName WorldContextHandle) const;

private:
	static constexpr double UpdateInterval = 0.1;

	double LastUpdateTime = 0.0;

	bool bContinuousUpdate = false;
	bool bTrackSelection = false;

#if WITH_EDITOR
	FDelegateHandle SelectionChangedHandle;
#endif
	bool bSelectionStopped = false;

	FName SelectedWorldContextHandle;
	FText SelectedWorldTitle;
	TArray<TWeakObjectPtr<UAbilitySystemComponent>> AbilitySystemComponents;
	TWeakObjectPtr<UAbilitySystemComponent> SelectedComponent;
	FText SelectedComponentTitle;

private:
	TSharedPtr<FTabManager> TabManager;
	TMap<FName, TWeakPtr<SDockTab>> SpawnedTabs;

	TSharedPtr<SGASAbilitiesTab> AbilitiesTab;
	TSharedPtr<SGASAttributesTab> AttributesTab;
	TSharedPtr<SGASGameplayEffectsTab> GameplayEffectsTab;
	TSharedPtr<SGASGameplayTagsTab> GameplayTagsTab;
};