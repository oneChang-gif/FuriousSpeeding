#include "Item/CoinItem.h"
#include "CXPawn.h"
#include "PlayerState/CXPlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACoinItem::ACoinItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.0f);
	SetMinNetUpdateFrequency(1.0f);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(SceneRoot);
	OverlapSphere->InitSphereRadius(100.0f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	OverlapSphere->SetGenerateOverlapEvents(true);
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(SceneRoot);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACoinItem::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		OverlapSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
		TArray<AActor*> Vehicles;
		OverlapSphere->GetOverlappingActors(Vehicles, ACXPawn::StaticClass());
		for (AActor* Actor : Vehicles)
		{
			if (TryPickup(Cast<ACXPawn>(Actor)))
			{
				break;
			}
		}
		return;
	}

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACoinItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryPickup(Cast<ACXPawn>(OtherActor));
}

bool ACoinItem::TryPickup(ACXPawn* Vehicle)
{
	if (!HasAuthority() || bCollected || IsActorBeingDestroyed() || !IsValid(Vehicle))
	{
		return false;
	}
	ACXPlayerState* State = Vehicle->GetPlayerState<ACXPlayerState>();
	if (!IsValid(State) || CoinAmount <= 0)
	{
		return false;
	}
	if (CoinAmount > MAX_int32 - State->GetCoins())
	{
		return false;
	}
	bCollected = true;
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	State->AddCoins(CoinAmount);
	MulticastPickup(GetActorLocation());
	Destroy();
	return true;
}

void ACoinItem::MulticastPickup_Implementation(FVector Location)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (IsValid(PickupParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupParticle, Location);
	}
	if (IsValid(PickupSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, Location);
	}
}
