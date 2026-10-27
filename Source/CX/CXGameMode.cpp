#include "CXGameMode.h"
#include "GameState/CXGameState.h"
#include "TimerManager.h"
#include "PlayerState/CXPlayerState.h"
#include "CXPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Subsystem/ChatSubsystem.h" 

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (IsValid(NewPlayer) == false)
	{
		return;
	}

	if (ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer))
	{
		if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
		{
			const TArray<FChatData> ChatLog = ChatSubsystem->GetChatLog();
			if (!ChatLog.IsEmpty())
			{
				CXPlayerController->STCChatLog(ChatLog);
			}
		}
	}
	Super::PostLogin(NewPlayer);
	if (!bCountdownStarted && NewPlayer->PlayerState && !NewPlayer->PlayerState->IsOnlyASpectator())
	{
		StartWaitingCountdown();
	}
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}

void ACXGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!bCountdownStarted)
	{
		if (ACXGameState* State = GetGameState<ACXGameState>())
		{
			State->SetGameState(EGameState::Waiting);
			State->SetRemainingTime(0);
		}
	}
}

void ACXGameMode::StartWaitingCountdown()
{
	ACXGameState* State = GetGameState<ACXGameState>();
	if (!HasAuthority() || bCountdownStarted || !ensureMsgf(State, TEXT("GameMode must use CXGameState.")))
	{
		return;
	}
	bCountdownStarted = true;
	State->SetWinners({});
	for (APlayerState* Player : State->PlayerArray)
	{
		if (ACXPlayerState* CXPlayer = Cast<ACXPlayerState>(Player))
		{
			CXPlayer->SetMatchResult(ECXMatchResult::Pending);
		}
	}
	State->SetGameState(EGameState::Waiting);
	State->SetRemainingTime(FMath::Max(1, WaitingDuration));
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ThisClass::OnWaitingCountdown, 1.0f, true);
}

void ACXGameMode::OnWaitingCountdown()
{
	ACXGameState* State = GetGameState<ACXGameState>();
	if (!State)
	{
		return;
	}
	State->SetRemainingTime(State->GetRemainingTime() - 1);
	if (State->GetRemainingTime() == 0)
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		StartPlaying();
	}
}

void ACXGameMode::StartPlaying()
{
	ACXGameState* State = GetGameState<ACXGameState>();
	if (!HasAuthority() || !State)
	{
		return;
	}
	State->SetGameState(EGameState::Playing);
	State->SetRemainingTime(FMath::Max(1, PlayingDuration));
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &ThisClass::OnPlayingCountdown, 1.0f, true);
}

void ACXGameMode::OnPlayingCountdown()
{
	ACXGameState* State = GetGameState<ACXGameState>();
	if (!State)
	{
		return;
	}
	State->SetRemainingTime(State->GetRemainingTime() - 1);
	if (State->GetRemainingTime() == 0)
	{
		FinishGame();
	}
}

void ACXGameMode::FinishGame()
{
	ACXGameState* State = GetGameState<ACXGameState>();
	if (!HasAuthority() || !State)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	TArray<ACXPlayerState*> NewWinners;
	int32 HighestCoins = -1;
	for (APlayerState* Player : State->PlayerArray)
	{
		ACXPlayerState* CXPlayer = Cast<ACXPlayerState>(Player);
		if (!IsValid(CXPlayer) || CXPlayer->IsOnlyASpectator() || CXPlayer->IsInactive())
		{
			continue;
		}
		const int32 Coins = CXPlayer->GetCoins();
		if (Coins > HighestCoins)
		{
			HighestCoins = Coins;
			NewWinners.Reset();
			NewWinners.Add(CXPlayer);
			continue;
		}

		if (Coins == HighestCoins)
		{
			NewWinners.Add(CXPlayer);
		}
	}
	State->SetWinners(NewWinners);
	for (APlayerState* Player : State->PlayerArray)
	{
		ACXPlayerState* CXPlayer = Cast<ACXPlayerState>(Player);
		if (!IsValid(CXPlayer) || CXPlayer->IsOnlyASpectator() || CXPlayer->IsInactive())
		{
			continue;
		}

		ECXMatchResult Result = ECXMatchResult::Defeat;
		if (NewWinners.Contains(CXPlayer))
		{
			Result = ECXMatchResult::Victory;
		}

		CXPlayer->SetMatchResult(Result);
	}
	State->SetRemainingTime(0);
	State->SetGameState(EGameState::Finished);
}

void ACXGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
	Super::EndPlay(EndPlayReason);
}
