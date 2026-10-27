#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "ChatUI.generated.h"

UCLASS()
class CX_API UChatUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);
	void SetChatLog(const TArray<struct FChatData>& ChatLog);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatScrollBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMultiLineEditableTextBox> MultilineEditableTextBox_ChatLog;
};
