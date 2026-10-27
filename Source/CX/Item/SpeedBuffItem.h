#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpeedBuffItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;
class ACXPawn;

UCLASS(Blueprintable)
class CX_API ASpeedBuffItem : public AActor
{
	GENERATED_BODY()

public:
	ASpeedBuffItem();
	bool TryPickup(ACXPawn* Vehicle);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects")
	TObjectPtr<UParticleSystem> PickupParticle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	float BonusSpeedAmount = 10.0f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickup(FVector Location);

private:
	bool bCollected = false;
};
