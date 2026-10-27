#include "Item/CXSponsoredItem.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACXSponsoredItem::ACXSponsoredItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
}

void ACXSponsoredItem::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SpawnItem();
	}
}

void ACXSponsoredItem::SpawnItem()
{
	if (!HasAuthority() || bEndingPlay || IsValid(SpawnedItem) || !IsValid(ItemClass) || !GetWorld())
	{
		return;
	}
	const FTransform Transform = GetActorTransform();
	AActor* Item = GetWorld()->SpawnActorDeferred<AActor>(ItemClass, Transform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(Item))
	{
		return;
	}
	SpawnedItem = Item;
	Item->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnItemDestroyed);
	Item->FinishSpawning(Transform);
}

void ACXSponsoredItem::OnItemDestroyed(AActor* DestroyedActor)
{
	if (!HasAuthority() || bEndingPlay || DestroyedActor != SpawnedItem)
	{
		return;
	}
	SpawnedItem = nullptr;
	const float Delay = FMath::IsFinite(SpawnInterval) ? FMath::Max(0.01f, SpawnInterval) : 2.0f;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnItem, Delay, false);
}

void ACXSponsoredItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	if (IsValid(SpawnedItem))
	{
		SpawnedItem->OnDestroyed.RemoveDynamic(this, &ThisClass::OnItemDestroyed);
		if (HasAuthority())
		{
			SpawnedItem->Destroy();
		}
	}
	SpawnedItem = nullptr;
	Super::EndPlay(EndPlayReason);
}
