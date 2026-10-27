#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "PlayerState/CXPlayerState.h"
#include "GameState/CXGameState.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Subsystem/ChatSubsystem.h"

void ACXPlayerController::STCChatLog_Implementation(const TArray<FChatData>& ChatLog)
{
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->SetChatLog(ChatLog);
		if (VehicleUI) VehicleUI->SetChatLog(ChatSubsystem->GetChatLog());
	}
}

void ACXPlayerController::CTSChatMessage_Implementation(const FString& Message)
{

	const FString CleanMessage = Message.Left(512).TrimStartAndEnd();
	if (!PlayerState || CleanMessage.IsEmpty()) return;
	const FString PlayerName = PlayerState->GetPlayerName();
	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		ChatSubsystem->AddChatMessage(PlayerName, CleanMessage);
	}

	for(TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
	{
		if (ACXPlayerController* PC = *It)
		{
			PC->STCChatMessage(PlayerName, CleanMessage);
		}
	}
}

void ACXPlayerController::STCChatMessage_Implementation(const FString& Sender, const FString& Message)
{

	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		// Listen-server history was already updated by the server RPC.
		if (!HasAuthority()) ChatSubsystem->AddChatMessage(Sender, Message);
		if (VehicleUI) VehicleUI->SetChatLog(ChatSubsystem->GetChatLog());
	}
}

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	ConsoleCommand(TEXT("DisableAllScreenMessages"), false);
	
	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);
	VehicleUI->AddToViewport();

	if (UChatSubsystem* ChatSubsystem = GetWorld()->GetSubsystem<UChatSubsystem>())
	{
		VehicleUI->SetChatLog(ChatSubsystem->GetChatLog());
	}
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);
	if (VehicleUI)
	{
		if (const ACXPlayerState* PS = GetPlayerState<ACXPlayerState>())
		{
			VehicleUI->UpdatePlayerStateInfo(PS->GetPlayerStateInfo());
		}
	}

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
	if (IsLocalController() && IsValid(VehicleUI))
	{
		ACXGameState* CXGameState =
			GetWorld()->GetGameState<ACXGameState>();

		if (IsValid(CXGameState))
		{
			VehicleUI->UpdateMatchStatus(
				CXGameState->GetGameState(), CXGameState->GetRemainingTime());
		}

		ACXPlayerState* LocalPlayerState =
			GetPlayerState<ACXPlayerState>();

		if (IsValid(CXGameState) && IsValid(LocalPlayerState))
		{
			const ECXMatchResult Result = LocalPlayerState->GetMatchResult();
			const bool bFinished =
				CXGameState->GetGameState() == EGameState::Finished &&
				Result != ECXMatchResult::Pending;

			const bool bWinner =
				Result == ECXMatchResult::Victory;

			VehicleUI->UpdateGameResult(bFinished, bWinner);
			return;
		}

		VehicleUI->UpdateGameResult(false, false);
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}
