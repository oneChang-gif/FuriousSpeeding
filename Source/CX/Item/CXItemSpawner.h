#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"
class UBillboardComponent;

UCLASS(Blueprintable)
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

	UFUNCTION(BlueprintPure, Category = "Spawner")
	AActor* GetSpawnedItem() const { return SpawnedItem; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AActor> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner", meta = (ClampMin = "0.01", Units = "s"))
	float SpawnInterval = 2.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<AActor> SpawnedItem;
	void SpawnItem();

	UFUNCTION()
	void OnItemDestroyed(AActor* DestroyedActor);

private:
	FTimerHandle SpawnTimerHandle;
	bool bEndingPlay = false;
};
