#include "GameState/CXGameState.h"
#include "PlayerState/CXPlayerState.h"
#include "Net/UnrealNetwork.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACXGameState, CurrentGameState);
	DOREPLIFETIME(ACXGameState, RemainingTime);
	DOREPLIFETIME(ACXGameState, Winners);
}

void ACXGameState::SetGameState(EGameState NewState)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentGameState = NewState;
	ForceNetUpdate();
}

void ACXGameState::SetRemainingTime(int32 Seconds)
{
	if (!HasAuthority())
	{
		return;
	}
	RemainingTime = FMath::Max(0, Seconds);
	ForceNetUpdate();
}

void ACXGameState::SetWinners(const TArray<ACXPlayerState*>& NewWinners)
{
	if (!HasAuthority())
	{
		return;
	}
	Winners.Reset();
	for (ACXPlayerState* Player : NewWinners)
	{
		if (IsValid(Player))
		{
			Winners.AddUnique(Player);
		}
	}
	ForceNetUpdate();
}

bool ACXGameState::IsWinner(ACXPlayerState* Player) const
{
	return IsValid(Player) && Winners.Contains(Player);
}
