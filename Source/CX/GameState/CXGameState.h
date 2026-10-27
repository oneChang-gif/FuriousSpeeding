#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

UENUM(BlueprintType)
enum class EGameState : uint8
{
	Waiting,
	Playing,
	Finished
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Match")
	EGameState GetGameState() const { return CurrentGameState; }

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetRemainingTime() const { return RemainingTime; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsWinner(ACXPlayerState* Player) const;
	void SetGameState(EGameState NewState);
	void SetRemainingTime(int32 Seconds);
	void SetWinners(const TArray<ACXPlayerState*>& NewWinners);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	TArray<TObjectPtr<ACXPlayerState>> Winners;

protected:

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EGameState CurrentGameState = EGameState::Waiting;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 RemainingTime = 0;
};
