#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameState/CXGameState.h"
#include "CXUI.generated.h"

class UChatUI;
class UTextBlock;

UCLASS(abstract)
class CX_API UCXUI : public UUserWidget
{
	GENERATED_BODY()
	
#pragma region ChatUI
public:
	void OnNewChatMessage(const FString& InSender, const FString& InMessage);
	void SetChatLog(const TArray<struct FChatData>& ChatLog);
	void UpdatePlayerStateInfo(const FString& Info);

protected:
	virtual void NativeConstruct() override;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerStateInfoText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UChatUI> UI_Chat;
#pragma endregion

protected:

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

public:

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

protected:

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);

public:
	void UpdateGameResult(bool bFinished, bool bWinner);
	void UpdateMatchStatus(EGameState GameState, int32 RemainingTime);

protected:
	UTextBlock* CreateMatchText(FName WidgetName, float Top);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GameStateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RemainingTimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;
};


