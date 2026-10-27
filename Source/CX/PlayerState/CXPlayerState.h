#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

UENUM(BlueprintType)
enum class ECXMatchResult : uint8
{
	Pending,
	Victory,
	Defeat
};

UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACXPlayerState();
	virtual void Tick(float DeltaSeconds) override;
	ECXMatchResult GetMatchResult() const { return MatchResult; }
	void SetMatchResult(ECXMatchResult NewResult);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	int32 GetCoins() const { return Coins; }

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	float GetBonusMaxSpeed() const { return BonusMaxSpeed; }

	UFUNCTION(BlueprintPure, Category = "Player Stats")
	FString GetPlayerStateInfo() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Player Stats")
	void AddCoins(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Player Stats")
	bool TryRemoveCoins(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Player Stats")
	void AddBonusMaxSpeed(float Amount);

	UFUNCTION(Server, Reliable)
	void CTSAddCoin(int32 Amount);

#if !UE_BUILD_SHIPPING
	void AddLocalTestCoins(int32 Amount);
#endif

private:
	UPROPERTY(EditDefaultsOnly, Category = "Player Stats", meta = (ClampMin = "0.0"))
	float SpeedRecoveryPerSecond = 5.0f;

	UPROPERTY(Replicated)
	ECXMatchResult MatchResult = ECXMatchResult::Pending;

	UPROPERTY(Replicated)
	int32 Coins = 0;

	UPROPERTY(Replicated)
	float BonusMaxSpeed = 0.0f;
};
