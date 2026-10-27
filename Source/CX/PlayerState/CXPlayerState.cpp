#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

ACXPlayerState::ACXPlayerState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACXPlayerState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || BonusMaxSpeed >= 0.0f)
	{
		return;
	}
	BonusMaxSpeed = FMath::Min(0.0f, BonusMaxSpeed + FMath::Max(0.0f, SpeedRecoveryPerSecond) * DeltaSeconds);
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ACXPlayerState, MatchResult, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACXPlayerState, BonusMaxSpeed, COND_OwnerOnly);
}

void ACXPlayerState::SetMatchResult(ECXMatchResult NewResult)
{
	if (HasAuthority() == false)
	{
		return;
	}

	MatchResult = NewResult;
	ForceNetUpdate();
}

FString ACXPlayerState::GetPlayerStateInfo() const
{
	return FString::Printf(TEXT("Coins: %d, BonusMaxSpeed: %.2f"), Coins, BonusMaxSpeed);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || Amount > MAX_int32 - Coins)
	{
		return;
	}
	Coins += Amount;
	ForceNetUpdate();
}

bool ACXPlayerState::TryRemoveCoins(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || Coins < Amount)
	{
		return false;
	}
	Coins -= Amount;
	ForceNetUpdate();
	return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (!HasAuthority() || !FMath::IsFinite(Amount) || !FMath::IsFinite(BonusMaxSpeed + Amount))
	{
		return;
	}
	BonusMaxSpeed += Amount;
	ForceNetUpdate();
}

void ACXPlayerState::CTSAddCoin_Implementation(int32 Amount)
{
#if !UE_BUILD_SHIPPING
	AddCoins(Amount);
#endif
}

#if !UE_BUILD_SHIPPING
void ACXPlayerState::AddLocalTestCoins(int32 Amount)
{
	if (Amount > 0 && Amount <= MAX_int32 - Coins)
	{
		Coins += Amount;
	}
}

namespace
{
	void TestCoins(const TArray<FString>& Args, UWorld* World, bool bUseServerRPC)
	{
		if (!World || Args.Num() != 1)
		{
			return;
		}
		int32 Amount = 0;
		if (!LexTryParseString(Amount, *Args[0]) || Amount <= 0)
		{
			return;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		ACXPlayerState* PS = PC ? PC->GetPlayerState<ACXPlayerState>() : nullptr;
		if (!PS)
		{
			return;
		}
		if (bUseServerRPC)
		{
			PS->CTSAddCoin(Amount);
			return;
		}

		PS->AddLocalTestCoins(Amount);
	}
	FAutoConsoleCommandWithWorldAndArgs AddCoinCommand(
		TEXT("cheat.addcoin"), TEXT("cheat.addcoin <amount>: grant test coins through the server (development only)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { TestCoins(Args, World, true); }), ECVF_Cheat);
	FAutoConsoleCommandWithWorldAndArgs AddCoinClientCommand(
		TEXT("cheat.addcoin.client"), TEXT("cheat.addcoin.client <amount>: change only the local copy for the lecture replication demonstration."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { TestCoins(Args, World, false); }), ECVF_Cheat);
	FAutoConsoleCommandWithWorldAndArgs AddCoinServerCommand(
		TEXT("cheat.addcoin.server"), TEXT("cheat.addcoin.server <amount>: grant test coins on the server."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { TestCoins(Args, World, true); }), ECVF_Cheat);
}
#endif
