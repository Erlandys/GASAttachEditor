#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/Views/STileView.h"

class UAbilitySystemComponent;

class FGASTagNode : public TSharedFromThis<FGASTagNode>
{
public:
	explicit FGASTagNode(const TWeakObjectPtr<UAbilitySystemComponent>& WeakComponent, const FGameplayTag& Tag, FName PropertyName);

	void Update();

	FText GetName() const { return Name; }
	FText GetToolTip() const { return ToolTip; }
	FString GetTagName() const { return TagName; }

private:
	FText GatherName() const;
	FText GatherToolTip() const;
	FString GatherTagName() const;

private:
	FText Name;
	FText ToolTip;
	FString TagName;

private:
	const TWeakObjectPtr<UAbilitySystemComponent> WeakComponent;
	const FGameplayTag Tag;
	const FName PropertyName;
};

class SGASTagView : public STileView<TSharedPtr<FGASTagNode>>
{
public:
	//~ Begin STileView Interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	//~ End STileView Interface
};

class SGASTagViewItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGASTagViewItem)
	{}
		SLATE_ARGUMENT(TSharedPtr<FGASTagNode>, TagNode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleOnClicked() const;

protected:
	TSharedPtr<FGASTagNode> TagNode;
	TSharedPtr<STextBlock> ShowTextTag;
};