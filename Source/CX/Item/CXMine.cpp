#include "Item/CXMine.h"
#include "CXPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACXMine::ACXMine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	NetUpdateFrequency = 10.0f;
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	SetRootComponent(OverlapSphere);
	OverlapSphere->InitSphereRadius(100.0f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	OverlapSphere->SetGenerateOverlapEvents(true);
	MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
	MineMesh->SetupAttachment(OverlapSphere);
	MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACXMine::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlap);
	TArray<AActor*> Vehicles;
	OverlapSphere->GetOverlappingActors(Vehicles, ACXPawn::StaticClass());
	for (AActor* Vehicle : Vehicles)
	{
		Explode(Cast<ACXPawn>(Vehicle));
	}
}

void ACXMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACXMine, bExploded);
}

void ACXMine::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Explode(Cast<ACXPawn>(OtherActor));
}

void ACXMine::Explode(ACXPawn* Vehicle)
{
	if (!HasAuthority() || bExploded || !IsValid(Vehicle))
	{
		return;
	}
	bExploded = true;
	Vehicle->DisableThrottle();
	OnRep_Exploded();
	ForceNetUpdate();
	SetLifeSpan(1.0f);
}

void ACXMine::OnRep_Exploded()
{
	if (!bExploded || bExplosionPlayed)
	{
		return;
	}
	bExplosionPlayed = true;
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MineMesh->SetVisibility(false, true);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (IsValid(ExplosionParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, GetActorLocation(), GetActorRotation());
	}
	if (IsValid(ExplosionSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
}
