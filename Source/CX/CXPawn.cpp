#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "PlayerState/CXPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Particles/ParticleSystem.h"
#include "Item/CXMine.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameState/CXGameState.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));
	GetMesh()->SetGenerateOverlapEvents(true);

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
	bReplicates = true;
	AttackAction = CreateDefaultSubobject<UInputAction>(TEXT("AttackAction"));
	AttackAction->ValueType = EInputActionValueType::Boolean;
	PlaceMineAction = CreateDefaultSubobject<UInputAction>(TEXT("PlaceMineAction"));
	PlaceMineAction->ValueType = EInputActionValueType::Boolean;
	MineClass = ACXMine::StaticClass();

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACXPawn::InputAttack);
		EnhancedInputComponent->BindAction(PlaceMineAction, ETriggerEvent::Started, this, &ACXPawn::InputPlaceMine);
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (!AttackInputMapping)
				{
					AttackInputMapping = NewObject<UInputMappingContext>(this);
					AttackInputMapping->MapKey(AttackAction, EKeys::LeftMouseButton);
					AttackInputMapping->MapKey(PlaceMineAction, EKeys::F);
				}
				InputSubsystem->AddMappingContext(AttackInputMapping, 1);
			}
		}
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Canceled, this, &ACXPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
		return;
	}

	UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);
	UpdateGameStateInputLock();
	if (bThrottleDisabled)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
	}

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	UpdateGameStateInputLock();
	if (bGameStateInputLocked)
	{
		return;
	}
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	UpdateGameStateInputLock();
	if (bGameStateInputLocked)
	{
		return;
	}
	if (bThrottleDisabled)
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		return;
	}
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	UpdateGameStateInputLock();
	if (bGameStateInputLocked)
	{
		return;
	}
	bHandbrakePressed = true;
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	bHandbrakePressed = false;
	UpdateGameStateInputLock();
	if (bGameStateInputLocked)
	{
		return;
	}
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

}

void ACXPawn::InputAttack(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}
	if (IsLocallyControlled())
	{
		CTSAttack();
	}
}

void ACXPawn::CTSAttack_Implementation()
{
	if (!IsGamePlaying())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !GetController())
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Now - LastAttackTime < AttackCooldown)
	{
		return;
	}
	LastAttackTime = Now;

	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * AttackRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CXAttack), false, this);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	ACXPawn* Victim = bHit ? Cast<ACXPawn>(Hit.GetActor()) : nullptr;
	if (Victim)
	{
		UGameplayStatics::ApplyPointDamage(Victim, 5.0f, GetActorForwardVector(), Hit, GetController(), this, UDamageType::StaticClass());
	}
	MulticastAttackEffect(Start, End, Victim != nullptr, Hit.ImpactPoint, Hit.ImpactNormal);
}

float ACXPawn::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);
	if (!HasAuthority() || !Attacker || Attacker == this || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	ACXPlayerState* VictimState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerState = Attacker->GetPlayerState<ACXPlayerState>();
	if (!VictimState || !AttackerState || VictimState == AttackerState)
	{
		return 0.0f;
	}
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (!FMath::IsFinite(ActualDamage) || ActualDamage < 1.0f || ActualDamage >= static_cast<float>(MAX_int32))
	{
		return 0.0f;
	}
	const int32 Amount = static_cast<int32>(ActualDamage);
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (IsWheelBone(PointDamage.HitInfo.BoneName))
		{
			VictimState->AddBonusMaxSpeed(-FMath::Max(0.0f, WheelSpeedPenalty));
		}
	}
	if (AttackerState->GetCoins() > MAX_int32 - Amount || !VictimState->TryRemoveCoins(Amount))
	{
		return 0.0f;
	}
	AttackerState->AddCoins(Amount);
	return ActualDamage;
}

void ACXPawn::MulticastAttackEffect_Implementation(FVector Start, FVector End, bool bHitVehicle, FVector ImpactPoint, FVector ImpactNormal)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 3.0f);
	if (bHitVehicle && IsValid(HitParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticle, ImpactPoint, ImpactNormal.Rotation());
	}
}

void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ACXPawn, bThrottleDisabled, COND_OwnerOnly);
}

bool ACXPawn::IsWheelBone(FName BoneName) const
{
	if (BoneName.IsNone())
	{
		return false;
	}
	for (const FChaosWheelSetup& Wheel : ChaosVehicleMovement->WheelSetups)
	{
		if (Wheel.BoneName == BoneName)
		{
			return true;
		}
	}
	return false;
}

void ACXPawn::InputPlaceMine(const FInputActionValue& Value)
{
	if (!IsGamePlaying())
	{
		return;
	}
	if (IsLocallyControlled())
	{
		CTSPlaceMine();
	}
}

void ACXPawn::CTSPlaceMine_Implementation()
{
	if (!IsGamePlaying())
	{
		return;
	}
	if (!GetWorld() || !GetController() || !MineClass)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMineTime < MineCooldown)
	{
		return;
	}
	FVector Location = GetActorLocation() - GetActorForwardVector() * MineSpawnDistance;
	FHitResult GroundHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CXMineGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, Location + FVector(0.0f, 0.0f, 100.0f),
		Location - FVector(0.0f, 0.0f, 500.0f), ECC_WorldStatic, Params))
	{
		Location = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 15.0f);
	}
	const FTransform Transform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), Location);
	ACXMine* Mine = GetWorld()->SpawnActorDeferred<ACXMine>(MineClass, Transform, this, this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(Mine))
	{
		return;
	}
	LastMineTime = Now;
	Mine->FinishSpawning(Transform);
}

void ACXPawn::DisableThrottle()
{
	if (!HasAuthority())
	{
		return;
	}
	bThrottleDisabled = true;
	OnRep_ThrottleDisabled();
	STCSetThrottleDisabled(true);
	ForceNetUpdate();
	GetWorldTimerManager().SetTimer(ThrottleDisabledTimer, this, &ThisClass::RestoreThrottle, 2.0f, false);
}

void ACXPawn::RestoreThrottle()
{
	bThrottleDisabled = false;
	STCSetThrottleDisabled(false);
	ForceNetUpdate();
}

void ACXPawn::STCSetThrottleDisabled_Implementation(bool bDisabled)
{
	bThrottleDisabled = bDisabled;
	OnRep_ThrottleDisabled();
}

void ACXPawn::OnRep_ThrottleDisabled()
{
	if (bThrottleDisabled && IsValid(ChaosVehicleMovement))
	{
		ChaosVehicleMovement->SetThrottleInput(0.0f);
	}
}

void ACXPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ThrottleDisabledTimer);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (AttackInputMapping)
			{
				InputSubsystem->RemoveMappingContext(AttackInputMapping);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ACXPawn::BeginPlay()
{
	Super::BeginPlay();
	UpdateGameStateInputLock();
}

bool ACXPawn::IsGamePlaying() const
{
	if (!GetWorld())
	{
		return false;
	}
	const ACXGameState* State = GetWorld()->GetGameState<ACXGameState>();
	return IsValid(State) && State->GetGameState() == EGameState::Playing;
}

void ACXPawn::UpdateGameStateInputLock()
{
	if (!IsGamePlaying())
	{
		bGameStateInputLocked = true;
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		ChaosVehicleMovement->SetSteeringInput(0.0f);
		ChaosVehicleMovement->SetHandbrakeInput(true);
		return;
	}

	if (bGameStateInputLocked)
	{
		bGameStateInputLocked = false;
		ChaosVehicleMovement->SetHandbrakeInput(bHandbrakePressed);
	}
}

#undef LOCTEXT_NAMESPACE
